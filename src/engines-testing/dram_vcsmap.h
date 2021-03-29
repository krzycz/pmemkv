// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#pragma once

#define TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS 1

#include "../engine.h"
#include <tbb/concurrent_map.h>

#include <mutex>
#include <shared_mutex>

namespace pmem
{
namespace kv
{

class dram_vcsmap : public engine_base {
private:
	using key_t = std::string;
	using value_t = std::string;
	using container_t = tbb::concurrent_map<key_t, value_t>;

	using global_mutex_type = std::shared_timed_mutex;
	using shared_global_lock_type = std::shared_lock<global_mutex_type>;
	using unique_global_lock_type = std::unique_lock<global_mutex_type>;

	global_mutex_type mtx;

	container_t container = container_t();

public:
	dram_vcsmap(std::unique_ptr<internal::config> cfg);
	~dram_vcsmap();

	std::string name() final;

	status count_all(std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;
};

} /* namespace kv */
} /* namespace pmem */
