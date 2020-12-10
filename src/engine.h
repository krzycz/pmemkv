// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#ifndef LIBPMEMKV_ENGINE_H
#define LIBPMEMKV_ENGINE_H

#include <functional>
#include <memory>
#include <string>

#include "config.h"
#include "libpmemkv.hpp"

namespace pmem
{
namespace kv
{

class engine_base {
public:
	engine_base();

	virtual ~engine_base();

	static std::unique_ptr<engine_base>
	create_engine(const std::string &name, std::unique_ptr<internal::config> cfg);

	virtual std::string name() = 0;

	virtual status count_all(std::size_t &cnt);
	virtual status count_above(string_view key, std::size_t &cnt);
	virtual status count_equal_above(string_view key, std::size_t &cnt);
	virtual status count_equal_below(string_view key, std::size_t &cnt);
	virtual status count_below(string_view key, std::size_t &cnt);
	virtual status count_between(string_view key1, string_view key2,
				     std::size_t &cnt);

	virtual status get_all(get_kv_callback *callback, void *arg);
	virtual status get_above(string_view key, get_kv_callback *callback, void *arg);
	virtual status get_equal_above(string_view key, get_kv_callback *callback,
				       void *arg);
	virtual status get_equal_below(string_view key, get_kv_callback *callback,
				       void *arg);
	virtual status get_below(string_view key, get_kv_callback *callback, void *arg);
	virtual status get_between(string_view key1, string_view key2,
				   get_kv_callback *callback, void *arg);

	virtual status exists(string_view key);

	virtual status get(string_view key, get_v_callback *callback, void *arg) = 0;
	virtual status put(string_view key, string_view value) = 0;
	virtual status remove(string_view key) = 0;
	virtual status defrag(double start_percent, double amount_percent);
	virtual status stats(statistics *stat);

private:
	static void check_config_null(const std::string &engine_name,
				      std::unique_ptr<internal::config> &cfg);
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_ENGINE_H */
