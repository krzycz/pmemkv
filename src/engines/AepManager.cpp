#include "AepManager.hpp"

#include <libpmem.h>
#include <sys/mman.h>

#include <iostream>
#include <limits>
#include <thread>

#include "memset_nt.h"

thread_local int t_id = -1;

#ifdef DO_STATS
thread_local Stats stats;
thread_local uint64_t get_cnt = 0;
thread_local uint64_t set_cnt = 0;

void Stats::Print() {
#ifdef DE_LOG
    GlobalLogger.Print(
        "@@@ set stats: set_lru %llu set_aep %llu "
        "search_hash_in_set %llu search_free_list %llu write_value %llu "
        "set_lru_hash_table_in_set %llu\n"
        "@@@ get stats: get_lru %llu get_aep %llu "
        "get_offset %llu get_value %llu search_hash_in_get %llu, "
        "set_lru_hash_table_in_get %llu, search_lru_hash_table_in_get %llu\n",
        set_lru, set_aep, search_hash_in_set, search_free_list, write_value,
        set_lru_hash_table_in_set, get_lru, get_aep, get_offset, get_value,
        search_hash_in_get, set_lru_hash_table_in_get,
        search_lru_hash_table_in_get);
#endif
}
#endif

AepManager::AepManager() {}

AepManager::~AepManager() {}

uint8_t v_size_2_b_size[1025];

__attribute__((constructor(100))) static void init_v_size_2_b_size() {
    for (int i = 0; i <= 1024; i++) {
        int ret = i + KEY_SIZE + AEP_META_SIZE;
        v_size_2_b_size[i] =
            ret / AEP_BLOCK_SIZE + ((ret % AEP_BLOCK_SIZE == 0) ? 0 : 1);
    }
}

inline uint8_t get_block_size(uint16_t v_size) {
    return v_size_2_b_size[v_size];
}

// high | checksum(16) | v_size(16) | b_size(8) | version(8) | low

inline uint64_t encode_aep_meta(uint16_t v_size, uint8_t b_size,
                                uint8_t version, uint16_t checksum) {
    uint64_t ret = (((uint64_t)v_size) << 16) + (((uint64_t)b_size) << 8) +
                   ((uint64_t)version) + (((uint64_t)checksum) << 32);
    return ret;
}

inline void decode_aep_meta(uint64_t meta, uint16_t &v_size, uint8_t &b_size,
                            uint8_t &version, uint16_t &checksum) {
    version = meta & 255;
    meta >>= 8;
    b_size = meta & 255;
    meta >>= 8;
    v_size = meta & 65535;
    meta >>= 16;
    checksum = meta;
}

// high | b_off(32) | b_size(8) | version(8) |low

inline uint64_t encode_hash_meta(uint32_t b_off, uint16_t v_size,
                                 uint8_t b_size, uint8_t version) {
    uint64_t ret = (((uint64_t)b_off) << 32) + (((uint64_t)v_size) << 16) +
                   (((uint64_t)b_size) << 8) + ((uint64_t)version);
    return ret;
}

inline void decode_hash_meta(uint64_t meta, uint32_t &b_off, uint16_t &v_size,
                             uint8_t &b_size, uint8_t &version) {
    version = meta & 255;
    meta >>= 8;
    b_size = meta & 255;
    meta >>= 8;
    v_size = meta & 65535;
    meta >>= 16;
    b_off = meta;
}

void AepManager::Init(char *pmem_base) {
  pmem_base_ = pmem_base;

  // init hash
  dram_hash_map_ =
      (char *)mmap(nullptr, DRAM_SPARE_SIZE + DRAM_HASH_SIZE,
                   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  dram_spare_ = dram_hash_map_ + DRAM_HASH_SIZE;
  hash_bucket_entries_.resize(HASH_TOTAL_BUCKETS, 0);

  aep_value_log_ = pmem_base_;

  for (auto &fl : free_list_) {
    for (int i = AEP_MIN_BLOCK_SIZE; i < AEP_FREE_LIST_SLOT_NUM + 1; i++) {
      fl[i].reserve(10240);
    }
  }

  std::vector<std::thread> ths;
  for (uint32_t i = 0; i < THREAD_NUM; i += 1) {
    ths.emplace_back(std::thread(&AepManager::RestoreHashMap, this, i));
  }
  for (uint32_t i = 0; i < THREAD_NUM; i++)
    ths[i].join();

  if (restored_.load() == 0) {
    std::vector<std::thread> ths_init;
    for (int i = 0; i < THREAD_NUM; i++) {
      ths_init.emplace_back([=]() {
        // init pmem
        memset_movnt_sse2_clflushopt(aep_value_log_ +
                                         (uint64_t)i * PMEM_SIZE / THREAD_NUM,
                                     0, PMEM_SIZE / THREAD_NUM);

        // init hashmap
        memset(dram_hash_map_ + (uint64_t)i * DRAM_HASH_SIZE / THREAD_NUM, 0,
               DRAM_HASH_SIZE / THREAD_NUM);

        // init some spare space
        memset(dram_hash_map_ + DRAM_HASH_SIZE +
                   (uint64_t)i * DRAM_SPARE_SIZE / THREAD_NUM,
               0, 2LL * 1024 * 1024 * 1024 / THREAD_NUM);
      });
    }
    for (auto &t : ths_init)
      t.join();
  }
}

void AepManager::RestoreHashMap(uint32_t start) {
    uint64_t aep_meta; /* v_size | b_size | version */
    uint16_t aep_v_size;
    uint8_t aep_b_size;
    uint8_t aep_version;
    uint16_t aep_checksum;

    uint64_t hash_meta;
    uint32_t hash_b_off;
    uint16_t hash_v_size;
    uint8_t hash_b_size;
    uint8_t hash_version;

    uint32_t bucket;
    uint64_t key_hash_value;
    uint32_t entries;
    uint32_t slot;

    char key[KEY_SIZE];

    char *bucket_base;
    char *entry_base;
    char *block_base;

    int cnt = 0;
    spare_head_[start] =
        ((uint64_t)start * DRAM_SPARE_SIZE / THREAD_NUM) / HASH_BUCKET_SIZE;
    block_base = aep_value_log_ + PMEM_SIZE / THREAD_NUM * (uint64_t)start;
    while (1) {
        memcpy_6(&aep_meta, block_base);
        if (aep_meta == 0) break;
        decode_aep_meta(aep_meta, aep_v_size, aep_b_size, aep_version,
                        aep_checksum);
        memcpy_16(key, block_base + AEP_META_SIZE);
        key_hash_value = hash_key(key);
        uint16_t checksum = get_checksum(block_base + AEP_META_SIZE + KEY_SIZE,
                                         aep_v_size, key_hash_value);
        if (aep_checksum != checksum) {
            // data corrupt
            block_base += (uint64_t)aep_b_size * AEP_BLOCK_SIZE;
            continue;
        }
        cnt++;

        bucket = get_bucket_num(key_hash_value);
        bucket_base = dram_hash_map_ + (uint64_t)bucket * HASH_BUCKET_SIZE;
        slot = get_slot_num(bucket);
        entry_base = bucket_base;
        entries = hash_bucket_entries_[bucket];

        // update hashtable.
        {
            std::lock_guard<SpinMutex> lg(spins_[slot]);
            bool is_found = false;
            for (uint32_t i = 0; i < entries; i++) {
                if (memcmp_16(entry_base, key) == 0) {
                    is_found = true;
                    memcpy_8(&hash_meta, entry_base + KEY_SIZE);
                    decode_hash_meta(hash_meta, hash_b_off, hash_v_size,
                                     hash_b_size, hash_version);
                    if (hash_version < aep_version) {
                        is_found = false;
                    }

                    break;
                }

                entry_base += HASH_ENTRY_SIZE;
                if ((i + 1) % HASH_BUCKET_ENTRY_NUM == 0) {
                    uint32_t s_off;
                    if (i + 1 == entries) {
                        // alloc new bucket
                        s_off = spare_head_[start];
                        spare_head_[start] += 1;
#ifdef DO_LOG
                        if (s_off >= DRAM_SPARE_SIZE / HASH_BUCKET_SIZE) {
                            GlobalLogger.Print("SPARE OVERFLOW! \n");
                            exit(1);
                        }
#endif
                        // ptr to next bucket
                        memcpy_4(bucket_base + HASH_BUCKET_SIZE - 4, &s_off);
                    } else {
                        memcpy_4(&s_off, bucket_base + HASH_BUCKET_SIZE - 4);
                    }
                    bucket_base =
                        dram_spare_ + (uint64_t)s_off * HASH_BUCKET_SIZE;
                    entry_base = bucket_base;
                }
            }

            if (!is_found) {
                hash_meta = encode_hash_meta(
                    (block_base - aep_value_log_) / AEP_BLOCK_SIZE, aep_v_size,
                    aep_b_size, aep_version);
                memcpy_16(entry_base, key);
                memcpy_8(entry_base + KEY_SIZE, &hash_meta);
                hash_bucket_entries_[bucket] += 1;
            }
        }

        block_base += aep_b_size * AEP_BLOCK_SIZE;
    }
    aep_value_log_head_[start] =
        (block_base - (aep_value_log_ + PMEM_SIZE / THREAD_NUM * (uint64_t)start)) /
        AEP_BLOCK_SIZE;
    restored_.fetch_add(cnt);
#ifdef DO_LOG
    GlobalLogger.Print("restore cnt %d %d\n", start, cnt);
#endif
}

Status AepManager::GetAEP(const Slice &key, std::string *value,
                          uint32_t key_hash_value) {
    if (t_id < 0) {
        t_id = threads_.fetch_add(1, std::memory_order_relaxed);
        t_id %= THREAD_NUM;
    }
#ifdef DO_STATS
    if (t_id == 10 && get_cnt++ % 100000 == 0) {
        stats.Print();
    }
    StopWatch sw1(stats.get_aep);
#endif

    uint32_t bucket = get_bucket_num(key_hash_value);
    uint32_t slot = get_slot_num(bucket);
    uint32_t b_off = 0;
    uint16_t v_size = 0;
    char *block_base = NULL;

    uint64_t old_hash_meta;

    {
        uint64_t entries = hash_bucket_entries_[bucket];
        _mm_prefetch(&hash_cache_[slot], _MM_HINT_T0);

        char *entry_base = NULL;
        uint64_t hash_meta = 0;

        char *bucket_base =
            dram_hash_map_ + (uint64_t)bucket * HASH_BUCKET_SIZE;

        _mm_prefetch(bucket_base, _MM_HINT_T0);
        _mm_prefetch(bucket_base + 64, _MM_HINT_T0);

        entry_base = hash_cache_[slot].entry_base;

        if (entry_base != NULL &&
            memcmp_16(entry_base, key.data()) == 0) {
            memcpy_8(&hash_meta, entry_base + KEY_SIZE);
            old_hash_meta = hash_meta;
            hash_meta >>= 16;  // TODO: format

            // update hash cache
            v_size = (hash_meta & 65535);
            hash_meta >>= 16;
            b_off = hash_meta;

            block_base = aep_value_log_ + (uint64_t)b_off * AEP_BLOCK_SIZE;

            _mm_prefetch(block_base, _MM_HINT_T0);
            _mm_prefetch(block_base + 64, _MM_HINT_T0);
        } else {
            entry_base = bucket_base;

            for (uint32_t i = 0; i < entries; i++) {
                if (memcmp_16(entry_base, key.data()) == 0) {
                    memcpy_8(&hash_meta, entry_base + KEY_SIZE);
                    old_hash_meta = hash_meta;
                    hash_meta >>= 16;  // TODO: format

                    // update hash cache
                    v_size = (hash_meta & 65535);
                    hash_meta >>= 16;
                    b_off = hash_meta;

                    hash_cache_[slot].entry_base = entry_base;

                    block_base =
                        aep_value_log_ + (uint64_t)b_off * AEP_BLOCK_SIZE;

                    _mm_prefetch(block_base + 64, _MM_HINT_T0);

                    break;
                }

                if (i == entries - 1) break;

                entry_base += HASH_ENTRY_SIZE;
                if ((i + 1) % HASH_BUCKET_ENTRY_NUM == 0) {
                    // next bucket
                    uint32_t s_off;
                    memcpy_4(&s_off, bucket_base + HASH_BUCKET_SIZE - 4);
                    bucket_base =
                        dram_spare_ + (uint64_t)s_off * HASH_BUCKET_SIZE;

                    _mm_prefetch(bucket_base, _MM_HINT_T0);
                    _mm_prefetch(bucket_base + 64, _MM_HINT_T0);

                    entry_base = bucket_base;
                }
            }
        }

        Status s;
        if (block_base == NULL) {
            s = NotFound;
            return s;
        } else {
            while (1) {
                value->assign(block_base + KEY_SIZE + AEP_META_SIZE, v_size);
                std::atomic_thread_fence(std::memory_order_acquire);
                uint64_t new_hash_meta;
                memcpy_8(&new_hash_meta, entry_base + KEY_SIZE);
                if (__glibc_likely(old_hash_meta == new_hash_meta)) break;

                // Double check, 防止读取的value被更新后，空间被复用给其他value。
                old_hash_meta = new_hash_meta;
                new_hash_meta >>= 16;  // TODO: format
                v_size = (new_hash_meta & 65535);
                new_hash_meta >>= 16;
                b_off = new_hash_meta;
                block_base = aep_value_log_ + (uint64_t)b_off * AEP_BLOCK_SIZE;
            }
            return Ok;
        }
    }
}

Status AepManager::SetAEP(const Slice &key, const char *value,
                          uint16_t new_hash_v_size, uint32_t key_hash_value,
                          uint64_t checksum) {
    if (t_id < 0) {
        t_id = threads_.fetch_add(1, std::memory_order_relaxed);
        t_id %= THREAD_NUM;
    }

#ifdef DO_STATS
    set_cnt++;
    if (t_id == 10 && set_cnt++ % 100000 == 0) {
        stats.Print();
    }
    StopWatch sw1(stats.set_aep);
#endif
    uint32_t bucket = get_bucket_num(key_hash_value);
    uint32_t slot = get_slot_num(bucket);
    uint64_t entries = hash_bucket_entries_[bucket];

    // save entries to hashtable

    char *bucket_base = dram_hash_map_ + (uint64_t)bucket * HASH_BUCKET_SIZE;

    _mm_prefetch(bucket_base, _MM_HINT_T0);
    _mm_prefetch(bucket_base + 64, _MM_HINT_T0);

    char *entry_base = bucket_base;

    bool is_found = false;
    uint64_t hash_meta;
    uint32_t hash_b_off = 0;
    uint16_t hash_v_size = 0;
    uint8_t hash_b_size = 0;  // 占用的存储空间大小而不是 value 的实际大小！！！
    uint8_t hash_version = 0;

    uint8_t new_hash_b_size = get_block_size(new_hash_v_size);
    // 获取空间是线程内，不需要加锁
    uint32_t new_hash_b_off = SetValueOffset(new_hash_v_size, new_hash_b_size);

    char *block_base;
    block_base = aep_value_log_ + (uint64_t)new_hash_b_off * AEP_BLOCK_SIZE;

    {
        // 以下需要保证 setvalue 完成前不会由于另一线程更新该 key
        // 导致写的空间被释放后重新分配至另一线程的 free_list
        // 中，所以需要全部加锁
        std::lock_guard<SpinMutex> lg(spins_[slot]);

        {
#ifdef DO_STATS
            StopWatch sw2(stats.search_hash_in_set);
#endif
            // search hashtable to update

            for (uint32_t i = 0; i < entries; i++) {
                if (memcmp_16(entry_base, key.data()) == 0) {
                    is_found = true;
                    memcpy_8(&hash_meta, entry_base + KEY_SIZE);
                    decode_hash_meta(hash_meta, hash_b_off, hash_v_size,
                                     hash_b_size, hash_version);

                    break;
                }

                entry_base += HASH_ENTRY_SIZE;
                if ((i + 1) % HASH_BUCKET_ENTRY_NUM == 0) {
                    // next bucket
                    uint32_t s_off;
                    if (i + 1 == entries) {
                        // alloc new bucket
                        s_off = spare_head_[t_id];
                        spare_head_[t_id] += 1;
#ifdef DO_LOG
                        if (s_off >= DRAM_SPARE_SIZE / HASH_BUCKET_SIZE) {
                            GlobalLogger.Print("SPARE OVERFLOW! \n");
                            exit(1);
                        }
#endif
                        // ptr to next bucket
                        memcpy_4(bucket_base + HASH_BUCKET_SIZE - 4, &s_off);
                    } else {
                        memcpy_4(&s_off, bucket_base + HASH_BUCKET_SIZE - 4);
                    }
                    bucket_base =
                        dram_spare_ + (uint64_t)s_off * HASH_BUCKET_SIZE;

                    _mm_prefetch(bucket_base, _MM_HINT_T0);
                    _mm_prefetch(bucket_base + 64, _MM_HINT_T0);

                    entry_base = bucket_base;
                }
            }
        }

        uint8_t new_hash_version = (is_found) ? hash_version + 1 : 1;
        uint64_t new_hash_meta = encode_hash_meta(
            new_hash_b_off, new_hash_v_size, new_hash_b_size, new_hash_version);

        uint64_t aep_meta = encode_aep_meta(new_hash_v_size, new_hash_b_size,
                                            new_hash_version, checksum);
        memcpy_6(block_base, &aep_meta);
        memcpy_16(block_base + AEP_META_SIZE, key.data());
        memcpy(block_base + AEP_META_SIZE + KEY_SIZE, value, new_hash_v_size);
        pmem_persist(block_base, AEP_META_SIZE + KEY_SIZE + new_hash_v_size);
        
        memcpy_8(entry_base + KEY_SIZE, &new_hash_meta);

        if (!is_found) {
            // add entry
            memcpy_16(entry_base, key.data());
            hash_bucket_entries_[bucket]++;
        } else {
            // update free_list
            free_list_[t_id][hash_b_size].push_back(hash_b_off);
        }
    }
    return Ok;
}

uint32_t AepManager::SetValueOffset(uint16_t v_size, uint8_t &b_size) {
    uint32_t b_off = 0;
    bool should_alloc = true;
    b_off = aep_value_log_head_[t_id];
    bool full_log =
        ((uint64_t)b_off + b_size >= PMEM_SIZE / AEP_BLOCK_SIZE / THREAD_NUM);
    if (full_log) {
#ifdef DO_STATS
        StopWatch sw2(stats.search_free_list);
#endif
        for (uint8_t i = b_size; i < AEP_FREE_LIST_SLOT_NUM; i++) {
            if (free_list_[t_id][i].size() != 0) {
                b_off = *free_list_[t_id][i].rbegin();
                free_list_[t_id][i].pop_back();
                b_size = i;

                should_alloc = false;
                break;
            }
        }
    }

    if (should_alloc) {
#ifdef DO_LOG
        if (full_log) {
            GlobalLogger.Print("PMEM OVERFLOW \n");
            exit(1);
        }
#endif
        aep_value_log_head_[t_id] += b_size;
        b_off += t_id * (PMEM_SIZE / AEP_BLOCK_SIZE / THREAD_NUM);
    }
    return b_off;
}