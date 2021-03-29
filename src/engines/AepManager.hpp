#pragma once

#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "Logger.hpp"
#include "Utils.hpp"
#include "atomic"
#include "cassert"
#include "db.hpp"
#include "list"
#include "unordered_map"

class AepManager {
public:
    AepManager();
    ~AepManager();

    void Init(char *pmem_base);

    // Get/Set From AEP
    Status GetAEP(const Slice &key, std::string *value,
                  uint32_t key_hash_value);
    Status SetAEP(const Slice &key, const char *value, uint16_t new_hash_v_size,
                  uint32_t key_hash_value, uint64_t checksum);

    uint32_t aep_value_log_head_[THREAD_NUM];
    uint32_t spare_head_[THREAD_NUM];
    std::atomic<int> threads_{0};
    std::atomic<uint64_t> restored_{0};

    std::vector<uint64_t> found_{};
    std::vector<uint64_t> not_found_{};

private:
    uint32_t SetValueOffset(uint16_t v_size, uint8_t &b_size);

    void RestoreHashMap(uint32_t start);

    char *pmem_base_;
    char *aep_value_log_;  // start position to store values

    // HashTable
    char *dram_hash_map_;
    char *dram_spare_;

    std::vector<uint64_t> hash_bucket_entries_;

    std::vector<uint32_t> free_list_[THREAD_NUM][AEP_FREE_LIST_SLOT_NUM];

    struct HashCache {
        char *entry_base = NULL;
    };

    HashCache hash_cache_[SLOT_NUM];
    SpinMutex spins_[SLOT_NUM];
    // cache
};