// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "fmap.h"
#include "../out.h"

#include <unistd.h>
#include <libpmem.h>

namespace pmem
{
namespace kv
{

fmap::fmap(std::unique_ptr<internal::config> cfg)
{
	const char *path;
	if (!cfg->get_string("path", &path))
		throw internal::invalid_argument(
			"Config does not contain item with key: \"path\"");

	this->Init((std::string)path);
}

fmap::~fmap() { pmem_unmap(pmem_base_, mapped_len_); }

std::string fmap::name()
{
	return "fmap";
}

void fmap::Init(const std::string &name) {
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

status fmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));	
	auto value = reinterpret_cast<std::string *>(arg);

	Slice k;
	k.data() = key.data();
	k.size() = 16;

	uint64_t key_hash_value = hash_key(key.data());
	aep_.GetAEP(k, value, key_hash_value);

	return status::OK;
}

status fmap::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));

	uint64_t key_hash_value = hash_key(key.data());
	uint64_t checksum =
	get_checksum(value.data(), value.size(), key_hash_value);

	Slice k;
	k.data() = key.data();
	k.size() = 16;
	aep_.SetAEP(k, value.data(), value.size(), key_hash_value, checksum);

	return status::OK;
}

status fmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));

	return status::OK;
}

} // namespace kv
} // namespace pmem
