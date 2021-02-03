// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#pragma once

#include "../iterator.h"
#include "../pmemobj_engine.h"
#include "../polymorphic_string.h"

#include <libpmemobj++/container/concurrent_hash_map.hpp>
#include <libpmemobj++/persistent_ptr.hpp>

namespace pmem
{
namespace kv
{
namespace internal
{
namespace cmap
{

class key_equal {
public:
	template <typename M, typename U>
	bool operator()(const M &lhs, const U &rhs) const
	{
		return lhs == rhs;
	}
};

class string_hasher {
	/* hash multiplier used by fibonacci hashing */
	static const size_t hash_multiplier = 11400714819323198485ULL;

public:
	using transparent_key_equal = key_equal;

	size_t operator()(const pmem::kv::polymorphic_string &str) const
	{
		return hash(str.c_str(), str.size());
	}

	size_t operator()(string_view str) const
	{
		return hash(str.data(), str.size());
	}

private:
	size_t hash(const char *str, size_t size) const
	{
		size_t h = 0;
		for (size_t i = 0; i < size; ++i) {
			h = static_cast<size_t>(str[i]) ^ (h * hash_multiplier);
		}
		return h;
	}
};

using string_t = pmem::kv::polymorphic_string;
using map_t = pmem::obj::concurrent_hash_map<string_t, string_t, string_hasher>;

} /* namespace cmap */
} /* namespace internal */

class cmap : public pmemobj_engine_base<internal::cmap::map_t> {
	template <bool IsConst>
	class cmap_iterator;

public:
	cmap(std::unique_ptr<internal::config> cfg);
	~cmap();

	cmap(const cmap &) = delete;
	cmap &operator=(const cmap &) = delete;

	std::string name() final;

	status count_all(std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

	status defrag(double start_percent, double amount_percent) final;

	internal::iterator_base *new_iterator() final;
	internal::iterator_base *new_const_iterator() final;

private:
	void Recover(size_t reserve_count);
	internal::cmap::map_t *container;
};

template <>
class cmap::cmap_iterator<true> : virtual public internal::iterator_base {
	using container_type = internal::cmap::map_t;

public:
	cmap_iterator(container_type *container);

	status seek(string_view key) final;

	result<string_view> key() final;

	result<pmem::obj::slice<const char *>> read_range(size_t pos, size_t n) final;

protected:
	container_type *container;
	container_type::accessor acc_;
	pmem::obj::pool_base pop;
};

template <>
class cmap::cmap_iterator<false> : public cmap::cmap_iterator<true> {
	using container_type = internal::cmap::map_t;

public:
	cmap_iterator(container_type *container);

	result<pmem::obj::slice<char *>> write_range(size_t pos, size_t n) final;

	status commit() final;
	void abort() final;

private:
	std::vector<std::pair<std::string, size_t>> log;
};

} /* namespace kv */
} /* namespace pmem */
