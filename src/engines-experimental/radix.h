// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#ifndef LIBPMEMKV_RADIX_H
#define LIBPMEMKV_RADIX_H

#include "../comparator/pmemobj_comparator.h"
#include "../iterator.h"
#include "../pmemobj_engine.h"

#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/experimental/mpsc_queue.hpp>
#include <libpmemobj++/experimental/radix_tree.hpp>
#include <libpmemobj++/persistent_ptr.hpp>

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <shared_mutex>

namespace pmem
{
namespace kv
{
namespace internal
{
namespace radix
{

using key_value_type = pmem::obj::experimental::inline_string;

template <bool Mt>
using map_type_generic = pmem::obj::experimental::radix_tree<
	key_value_type, key_value_type,
	pmem::obj::experimental::bytes_view<key_value_type>, Mt>;

using map_type = map_type_generic<false>;
using map_mt_type = map_type_generic<true>;

using log_type = pmem::obj::experimental::mpsc_queue::pmem_log_type;

template <typename MapType = map_type>
struct pmem_type {
	pmem_type() : map()
	{
		std::memset(reserved, 0, sizeof(reserved));
	}

	MapType map;
	pmem::obj::persistent_ptr<log_type> log;
	uint64_t reserved[6];
};

static_assert(sizeof(pmem_type<map_type>) == sizeof(map_type) + 64, "");

class transaction : public ::pmem::kv::internal::transaction {
public:
	transaction(pmem::obj::pool_base &pop, map_type *container);
	status put(string_view key, string_view value) final;
	status remove(string_view key) final;
	status commit() final;
	void abort() final;

private:
	pmem::obj::pool_base &pop;
	dram_log log;
	map_type *container;
};

template <typename Value>
class ordered_cache {
private:
	using key_type = std::string;
	using dram_value_type = std::pair<key_type, std::atomic<const Value *>>;
	using dram_map_type =
		std::map<string_view, typename std::list<dram_value_type>::iterator>;

public:
	using lru_list_type = std::list<dram_value_type>;
	using iterator = typename dram_map_type::iterator;
	using value_type = std::atomic<const Value *>;

	ordered_cache(size_t max_size) : max_size(max_size)
	{
	}

	ordered_cache(const ordered_cache &) = delete;
	ordered_cache(ordered_cache &&) = delete;

	ordered_cache &operator=(const ordered_cache &) = delete;
	ordered_cache &operator=(ordered_cache &&) = delete;

	template <typename F>
	value_type *put(string_view key, const Value *v, F &&evict)
	{
		assert(map.size() == lru_list.size());

		auto it = map.find(key);
		if (it == map.end()) {
			if (lru_list.size() < max_size) {
				lru_list.emplace_front(
					std::piecewise_construct,
					std::forward_as_tuple(key.data(), key.size()),
					std::forward_as_tuple(v));
				auto lit = lru_list.begin();

				auto ret = map_put(lit->first, lit);

				assert(ret.second);

				it = ret.first;
			} else {
				auto lit = evict(lru_list);

				if (lit == lru_list.end())
					return nullptr;

				map.erase(lit->first);

				lru_list.splice(lru_list.begin(), lru_list, lit);
				lru_list.begin()->first.assign(key.data(), key.size());
				lru_list.begin()->second.store(v,
							       std::memory_order_release);
				lit = lru_list.begin();

				auto ret = map_put(lit->first, lit);
				assert(ret.second);

				it = ret.first;
			}
		} else {
			lru_list.splice(lru_list.begin(), lru_list, it->second);
			lru_list.begin()->second.store(v, std::memory_order_release);
		}

		return &it->second->second;
	}

	value_type *get(string_view key, bool promote)
	{
		assert(map.size() == lru_list.size());

		auto it = map.find(key);
		if (it == map.end()) {
			return nullptr;
		} else {
			if (promote)
				lru_list.splice(lru_list.begin(), lru_list, it->second);

			return &it->second->second;
		}
	}

	iterator begin()
	{
		return map.begin();
	}

	iterator end()
	{
		return map.end();
	}

	iterator lower_bound(string_view key)
	{
		return map.lower_bound(key);
	}

	iterator upper_bound(string_view key)
	{
		return map.upper_bound(key);
	}

private:
	std::pair<typename dram_map_type::iterator, bool>
	map_put(const typename dram_map_type::key_type &k,
		const typename dram_map_type::mapped_type &v)
	{
#if __cpp_lib_map_try_emplace
		return map.try_emplace(k, v);
#else
		return map.emplace(k, v);
#endif
	}

	lru_list_type lru_list;
	dram_map_type map;
	const size_t max_size;
};

} /* namespace radix */
} /* namespace internal */

/**
 * Radix tree engine backed by:
 * https://github.com/pmem/libpmemobj-cpp/blob/master/include/libpmemobj%2B%2B/experimental/radix_tree.hpp
 *
 * It is a sorted, singlethreaded engine. Unlike other sorted engines it does not support
 * custom comparator (the order is defined by the keys' representation).
 *
 * The implementation is a variation of a PATRICIA trie - the internal
 * nodes do not store the path explicitly, but only a position at which
 * the keys differ. Keys are stored entirely in leafs.
 *
 * More info about radix tree: https://en.wikipedia.org/wiki/Radix_tree
 */
class radix
    : public pmemobj_engine_base<internal::radix::pmem_type<internal::radix::map_type>> {
	template <bool IsConst>
	class radix_iterator;

public:
	radix(std::unique_ptr<internal::config> cfg);
	~radix();

	radix(const radix &) = delete;
	radix &operator=(const radix &) = delete;

	std::string name() final;

	status count_all(std::size_t &cnt) final;
	status count_above(string_view key, std::size_t &cnt) final;
	status count_equal_above(string_view key, std::size_t &cnt) final;
	status count_equal_below(string_view key, std::size_t &cnt) final;
	status count_below(string_view key, std::size_t &cnt) final;
	status count_between(string_view key1, string_view key2, std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;
	status get_above(string_view key, get_kv_callback *callback, void *arg) final;
	status get_equal_above(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_equal_below(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_below(string_view key, get_kv_callback *callback, void *arg) final;
	status get_between(string_view key1, string_view key2, get_kv_callback *callback,
			   void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

	internal::transaction *begin_tx() final;

	internal::iterator_base *new_iterator() final;
	internal::iterator_base *new_const_iterator() final;

private:
	using container_type = internal::radix::map_type;
	using pmem_type = internal::radix::pmem_type<container_type>;

	void Recover();
	int iterate_callback(const container_type::iterator &it,
			     get_kv_callback *callback, void *arg);
	status iterate(container_type::iterator begin, container_type::iterator last,
		       get_kv_callback *callback, void *arg);

	container_type *container;
	std::unique_ptr<internal::config> config;
};

/**
 * Heterogenous engine which implements DRAM cache on top of radix tree container.
 *
 * On put, data is first inserted to DRAM cache and appended to pmem log (mpsc_queue).
 * There is one background thread which consumes data from the log and erases/inserts
 * consumed elements to the radix tree.
 *
 * On get, dram cache is first checked. If looked-for element is found there, it is
 * returned to the user. On cache-miss, we search the radix_tree. Read operations on
 * radix_tree are protected by Epoch Based Reclamation mechanism.
 */
class heterogeneous_radix
    : public pmemobj_engine_base<
	      internal::radix::pmem_type<internal::radix::map_mt_type>> {
public:
	heterogeneous_radix(std::unique_ptr<internal::config> cfg);
	~heterogeneous_radix();

	heterogeneous_radix(const heterogeneous_radix &) = delete;
	heterogeneous_radix &operator=(const heterogeneous_radix &) = delete;

	std::string name() final;

	status count_all(std::size_t &cnt) final;
	status count_above(string_view key, std::size_t &cnt) final;
	status count_equal_above(string_view key, std::size_t &cnt) final;
	status count_equal_below(string_view key, std::size_t &cnt) final;
	status count_below(string_view key, std::size_t &cnt) final;
	status count_between(string_view key1, string_view key2, std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;
	status get_above(string_view key, get_kv_callback *callback, void *arg) final;
	status get_equal_above(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_equal_below(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_below(string_view key, get_kv_callback *callback, void *arg) final;
	status get_between(string_view key1, string_view key2, get_kv_callback *callback,
			   void *arg) final;

	status exists(string_view key) final;

	status put(string_view key, string_view value) final;

	status remove(string_view k) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

private:
	using container_type = internal::radix::map_mt_type;
	using pmem_type = internal::radix::pmem_type<container_type>;
	using uvalue_type = pmem::obj::experimental::inline_string;
	using cache_type = internal::radix::ordered_cache<uvalue_type>;
	using pmem_log_type = internal::radix::log_type;
	using unique_ptr_type = std::unique_ptr<const char[], void (*)(const char *)>;
	using dram_iterator = typename cache_type::iterator;
	using pmem_iterator = typename container_type::iterator;

	struct queue_entry {
		queue_entry(cache_type::value_type *dram_entry, string_view key_,
			    string_view value_);

		const uvalue_type &key() const;

		const uvalue_type &value() const;
		uvalue_type &value();

		cache_type::value_type *dram_entry;
		bool remove;
		char padding[7];
	};

	static_assert(sizeof(queue_entry) == 16);

	using pmem_queue_type = pmem::obj::experimental::mpsc_queue;

	struct merged_iterator {
		merged_iterator(heterogeneous_radix &hetero_radix, dram_iterator dram_it,
				pmem_iterator pmem_it);
		merged_iterator(const merged_iterator &) = default;

		merged_iterator &operator++();

		bool dereferenceable() const;

		string_view key() const;
		std::pair<unique_ptr_type, size_t> value() const;

	private:
		heterogeneous_radix &hetero_radix;

		dram_iterator dram_it;
		pmem_iterator pmem_it;

		enum class current_it { dram, pmem } curr_it;

		void set_current_it();
	};

	friend struct merged_iterator;

	merged_iterator merged_begin();
	merged_iterator merged_end();
	merged_iterator merged_lower_bound(string_view key);
	merged_iterator merged_upper_bound(string_view key);

	int iterate_callback(const merged_iterator &it, get_kv_callback *callback,
			     void *arg);

	void bg_work();
	cache_type::value_type *cache_put_with_evict(string_view key,
						     const uvalue_type *value);
	bool log_contains(const void *entry) const;
	void handle_oom_from_bg();
	void consume_queue_entry(pmem::obj::string_view item, bool);
	unique_ptr_type log_read_optimistically(cache_type::value_type *ptr,
						const uvalue_type *&) const;
	unique_ptr_type try_read_value(cache_type::value_type *ptr,
				       const uvalue_type *&value) const;

	/* Element was logically removed but there might be an older version on pmem. */
	static const uvalue_type *tombstone_volatile();

	/* Element logically removed from cache and from pmem. */
	static const uvalue_type *tombstone_persistent();

	std::unique_ptr<cache_type> cache;
	size_t cache_size = 64000000;
	size_t log_size = 1000000;

	std::atomic<bool> stopped;
	std::thread bg_thread;

	pmem::obj::pool_base pop;

	container_type *container;
	std::unique_ptr<container_type::ebr::worker> container_worker;

	pmem_log_type *log;
	std::unique_ptr<internal::config> config;

	std::mutex eviction_lock;
	std::condition_variable eviction_cv;

	std::mutex bg_lock;
	std::condition_variable bg_cv;
	std::atomic<std::exception_ptr *> bg_exception_ptr;

	std::unique_ptr<pmem_queue_type> queue;
	std::unique_ptr<pmem_queue_type::worker> queue_worker;
};

template <>
class radix::radix_iterator<true> : public internal::iterator_base {
	using container_type = radix::container_type;

public:
	radix_iterator(container_type *container);

	status seek(string_view key) final;
	status seek_lower(string_view key) final;
	status seek_lower_eq(string_view key) final;
	status seek_higher(string_view key) final;
	status seek_higher_eq(string_view key) final;

	status seek_to_first() final;
	status seek_to_last() final;

	status is_next() final;
	status next() final;
	status prev() final;

	result<string_view> key() final;

	result<pmem::obj::slice<const char *>> read_range(size_t pos, size_t n) final;

protected:
	container_type *container;
	container_type::iterator it_;
	pmem::obj::pool_base pop;
};

template <>
class radix::radix_iterator<false> : public radix::radix_iterator<true> {
	using container_type = radix::container_type;

public:
	radix_iterator(container_type *container);

	result<pmem::obj::slice<char *>> write_range(size_t pos, size_t n) final;

	status commit() final;
	void abort() final;

private:
	std::vector<std::pair<std::string, size_t>> log;
};

class radix_factory : public engine_base::factory_base {
public:
	std::unique_ptr<engine_base>
	create(std::unique_ptr<internal::config> cfg) override
	{
		check_config_null(get_name(), cfg);

		uint64_t dram_caching;
		if (cfg->get_uint64("dram_caching", &dram_caching) && dram_caching) {
			return std::unique_ptr<engine_base>(
				new heterogeneous_radix(std::move(cfg)));
		} else {
			return std::unique_ptr<engine_base>(new radix(std::move(cfg)));
		}
	};

	std::string get_name() override
	{
		return "radix";
	};
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_RADIX_H */
