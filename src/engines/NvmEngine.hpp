#pragma once

#include <cstdlib>
#include <memory>
#include <vector>

#include "AepManager.hpp"
#include "Utils.hpp"
#include "db.hpp"

__attribute__((constructor(100))) static void init() {
    putenv("PMEM_MOVNT_THRESHOLD=160");
}

class NvmEngine : DB {
public:
    static Status CreateOrOpen(const std::string &name, DB **dbptr);

    NvmEngine();
    ~NvmEngine();

    void Init(const std::string &name);

    // may return notfound
    Status Get(const Slice &key, std::string *value);

    Status Set(const Slice &key, const Slice &value);

private:
    AepManager aep_;
    std::string file_name_;

    char *pmem_base_;
    size_t mapped_len_;
    int is_pmem_;

#ifdef DO_LOG
    size_t set_cnt_;
    size_t get_cnt_;
#endif
};