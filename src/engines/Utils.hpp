#pragma once

#include <emmintrin.h>
#include <smmintrin.h>
#include <sys/time.h>

#include <atomic>
#include <vector>

#define XXH_INLINE_ALL
#include "xxhash.h"

// #define DO_LOG
#define DO_STATS

#define SHARD_NUM 2048

// PMEM_RELATIVE
#define PMEM_SIZE (256LL * 1024 * 1024 * 1024)

#define KEY_SIZE 16
#define HASH_BUCKET_SIZE 128
#define HASH_BUCKET_ENTRY_NUM 5
#define HASH_TOTAL_BUCKETS (1 << 25)
#define HASH_META_SIZE \
    8  // high | b_off(32) | v_size(16) | b_size(8) | version(8) | low
#define HASH_ENTRY_SIZE (KEY_SIZE + HASH_META_SIZE)

#define AEP_META_SIZE \
    6  // high | v_size(16) | b_size(8) | version(8) | checksum(16) | low
#define AEP_BLOCK_SIZE 32
#define AEP_FREE_LIST_SLOT_NUM (1024 / AEP_BLOCK_SIZE + 2)
#define AEP_MIN_BLOCK_SIZE 4

//redefine it since DRAM_HASH_SIZE = HASH_TOTAL_BUCKETS * HASH_BUCKET_SIZE
#define DRAM_HASH_SIZE (1LL * HASH_TOTAL_BUCKETS * HASH_BUCKET_SIZE)
#define DRAM_SPARE_SIZE (1LL * HASH_TOTAL_BUCKETS * HASH_BUCKET_SIZE)

#define THREAD_NUM 16

#define SLOT_GRAIN 8
#define SLOT_NUM (HASH_TOTAL_BUCKETS / SLOT_GRAIN)

#define HASH_CACHE_NUM 16

inline uint32_t get_shard_num(uint32_t key_hash_value) {
    return key_hash_value & (SHARD_NUM - 1);
}

inline uint32_t get_bucket_num(uint32_t key_hash_value) {
    return key_hash_value & (HASH_TOTAL_BUCKETS - 1);
}

inline uint32_t get_slot_num(uint32_t bucket_num) {
    return bucket_num / SLOT_GRAIN;
}

inline uint64_t hash_key(const char *key) { return XXH3_64bits(key, KEY_SIZE); }

inline uint64_t get_checksum(const char *value, uint16_t v_size,
                             uint64_t key_hash_value) {
    return XXH3_64bits_withSeed(value, v_size, key_hash_value);
}

inline int memcmp_16(const void *a, const void *b) {
#if 0
	return memcmp(a, b, KEY_SIZE);
#else
    register __m128i xmm0, xmm1;
    xmm0 = _mm_loadu_si128((__m128i *)(a));
    xmm1 = _mm_loadu_si128((__m128i *)(b));
    __m128i diff = _mm_xor_si128(xmm0, xmm1);
    if (_mm_testz_si128(diff, diff))
        return 0;  // equal
    else
 		return 1;  // non-equal
#endif
}

inline void memcpy_16(void *dst, const void *src) {
    __m128i m0 = _mm_loadu_si128(((const __m128i *)src) + 0);
    _mm_storeu_si128(((__m128i *)dst) + 0, m0);
}

inline void memcpy_8(void *dst, const void *src) {
    *((uint64_t *)dst) = *((uint64_t *)src);
}

inline void memcpy_6(void *dst, const void *src) {
    unsigned char *dd = ((unsigned char *)dst) + 6;
    const unsigned char *ss = ((const unsigned char *)src) + 6;
    *((uint32_t *)(dd - 6)) = *((uint32_t *)(ss - 6));
    *((uint16_t *)(dd - 2)) = *((uint16_t *)(ss - 2));
}

inline void memcpy_4(void *dst, const void *src) {
    *((uint32_t *)dst) = *((uint32_t *)src);
}

inline void memcpy_2(void *dst, const void *src) {
    *((uint16_t *)dst) = *((uint16_t *)src);
}

// from rocksdb
class SpinMutex {
    std::atomic_flag locked = ATOMIC_FLAG_INIT;

public:
    void lock() {
        while (locked.test_and_set(std::memory_order_acquire)) {
            asm volatile("pause");
        }
    }
    void unlock() { locked.clear(std::memory_order_release); }
};

#ifdef DO_STATS

class Timer {
public:
    void Start() { gettimeofday(&start, nullptr); }

    uint64_t End() {
        struct timeval end;
        gettimeofday(&end, nullptr);
        return (end.tv_sec - start.tv_sec) * 1000000 +
               (end.tv_usec - start.tv_usec);
    }

private:
    struct timeval start;
};

class StopWatch {
public:
    StopWatch(uint64_t &s) : stats(s) { timer.Start(); }
    ~StopWatch() { stats += timer.End(); }

private:
    Timer timer;
    uint64_t &stats;
};

struct Stats {
    uint64_t get_aep{0};
    uint64_t get_offset{0};
    uint64_t get_value{0};
    uint64_t get_lru{0};
    uint64_t search_hash_in_get{0};
    uint64_t set_lru_hash_table_in_get{0};
    uint64_t search_lru_hash_table_in_get{0};
    uint64_t set_nvm{0};
    uint64_t set_lru{0};
    uint64_t set_lru_hash_table_in_set{0};
    uint64_t set_aep{0};
    uint64_t write_value{0};
    uint64_t search_hash_in_set{0};
    uint64_t search_free_list{0};
    void Print();
};

#endif