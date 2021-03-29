// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "dram_vcsmap.h"
#include <iostream>
#include <stdexcept>

namespace pmem
{
namespace kv
{

dram_vcsmap::dram_vcsmap(std::unique_ptr<internal::config> cfg)
{
	LOG("Started ok");
}

dram_vcsmap::~dram_vcsmap()
{
	LOG("Stopped ok");
}

std::string dram_vcsmap::name()
{
	return "dram_vcsmap";
}

status dram_vcsmap::count_all(std::size_t &cnt)
{
	LOG("count_all");

	shared_global_lock_type lock(mtx);
	cnt = container.size();

	return status::OK;
}

status dram_vcsmap::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");

	for (auto &element : container) {
		shared_global_lock_type lock(mtx);
		auto key = element.first;
		auto value = element.second;
		auto ret = callback(key.data(), key.length(), value.data(),
				    value.length(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status dram_vcsmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));

	shared_global_lock_type lock(mtx);
	if (container.contains(key_t(key))) {
		return status::OK;
	}
	return status::NOT_FOUND;
}

status dram_vcsmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));

	shared_global_lock_type lock(mtx);
	auto it = container.find(key_t(key));
	if (it != container.end()) {
		auto value = it->second;
		callback(value.data(), value.length(), arg);
		return status::OK;
	}

	return status::NOT_FOUND;
}

status dram_vcsmap::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	auto k = key_t(key);
	auto v = value_t(value);
	shared_global_lock_type lock(mtx);
	auto it = container.find(k);
	if (it != container.end()) {
		it->second = v;
	} else {
		auto element = std::pair<key_t, value_t>(k, v);
		container.insert(element);
	}
	return status::OK;
}

status dram_vcsmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));

	unique_global_lock_type lock(mtx);
	auto it = container.find(key_t(key));
	if (it != container.end()) {
		container.unsafe_erase(it);
		return status::OK;
	}
	return status::NOT_FOUND;
}

} // namespace kv
} // namespace pmem
