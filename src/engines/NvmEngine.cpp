#include "NvmEngine.hpp"

#include <libpmem.h>

Status DB::CreateOrOpen(const std::string &name, DB **dbptr, FILE *log_file) {
#ifdef DO_LOG
    GlobalLogger.Init(log_file);
#endif
    return NvmEngine::CreateOrOpen(name, dbptr);
}

DB::~DB() {}

Status NvmEngine::CreateOrOpen(const std::string &name, DB **dbptr) {
    NvmEngine *db = new NvmEngine();
    // GlobalLogger.Print("file name: %s \n", name.c_str());
    db->Init(name);
    *dbptr = db;
    return Ok;
}

NvmEngine::NvmEngine() {}

void NvmEngine::Init(const std::string &name) {
    file_name_ = name;
    if ((pmem_base_ = (char *)pmem_map_file(file_name_.c_str(), PMEM_SIZE,
                                            PMEM_FILE_CREATE, 0666,
                                            &mapped_len_, &is_pmem_)) == NULL) {
        perror("Pmem map file failed");
        exit(1);
    }
    // GlobalLogger.Print("is pmem %d \n", is_pmem_);
    aep_.Init(pmem_base_);
}

Status NvmEngine::Get(const Slice &key, std::string *value) {
    uint64_t key_hash_value = hash_key(key.data());

#ifdef DO_LOG
    uint32_t id_ = get_shard_num(key_hash_value);
    if (id_ < 1 && get_cnt_++ % 1250 == 0) {
        GlobalLogger.Print("## shard %d do %d get ## %llu %llu\n", id_,
                           get_cnt_, aep_.aep_value_log_head_[0],
                           aep_.spare_head_[0]);
    }

    if (get_cnt_ >= 1000000) {
        GlobalLogger.Print("exit!\n");
        exit(1);
    }
#endif

    return aep_.GetAEP(key.data(), value, key_hash_value);
}

Status NvmEngine::Set(const Slice &key, const Slice &value) {
    uint64_t key_hash_value = hash_key(key.data());
    // StopWatch sw(stats[get_shard_num(key_hash_value)].set_nvm);
    uint64_t checksum =
        get_checksum(value.data(), value.size(), key_hash_value);

#ifdef DO_LOG
    uint32_t id_ = get_shard_num(key_hash_value);
    if (id_ < 1 && set_cnt_++ % 1250 == 0) {
        GlobalLogger.Print("!! shard %d do %d set !! %llu %llu\n", id_,
                           set_cnt_, aep_.aep_value_log_head_[0],
                           aep_.spare_head_[0]);
    }
#endif

    aep_.SetAEP(key, value.data(), value.size(), key_hash_value, checksum);
    return Ok;
}

NvmEngine::~NvmEngine() { pmem_unmap(pmem_base_, mapped_len_); }
