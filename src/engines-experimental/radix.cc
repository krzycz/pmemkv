// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "radix.h"
#include "../out.h"

namespace pmem
{
namespace kv
{
namespace internal
{
namespace radix
{
transaction::transaction(pmem::obj::pool_base &pop, map_type *container)
    : pop(pop), container(container)
{
}

status transaction::put(string_view key, string_view value)
{
	log.insert(key, value);
	return status::OK;
}

status transaction::remove(string_view key)
{
	log.remove(key);
	return status::OK;
}

status transaction::commit()
{
	auto insert_cb = [&](const dram_log::element_type &e) {
		auto result = container->try_emplace(e.first, e.second);

		if (result.second == false)
			result.first.assign_val(e.second);
	};

	auto remove_cb = [&](const dram_log::element_type &e) {
		container->erase(e.first);
	};

	pmem::obj::transaction::run(pop, [&] { log.foreach (insert_cb, remove_cb); });

	log.clear();

	return status::OK;
}

void transaction::abort()
{
	log.clear();
}
} /* namespace radix */
} /* namespace internal */

static constexpr size_t align_up(size_t size, size_t align)
{
	return ((size) + (align)-1) & ~((align)-1);
}

radix::radix(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg, "pmemkv_radix"), config(std::move(cfg))
{
	Recover();
	LOG("Started ok");
}

radix::~radix()
{
	LOG("Stopped ok");
}

std::string radix::name()
{
	return "radix";
}

template <typename Iterator, typename Callback, typename Predicate>
static status iterate_generic(Iterator first, Callback &&cb, Predicate &&p)
{
	for (auto it = first; p(it); ++it) {
		auto ret = cb(it);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status radix::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();
	cnt = container->size();

	return status::OK;
}

status radix::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->upper_bound(key);
	auto last = container->end();

	cnt = internal::distance(first, last);

	return status::OK;
}

status radix::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->lower_bound(key);
	auto last = container->end();

	cnt = internal::distance(first, last);

	return status::OK;
}

status radix::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->upper_bound(key);

	cnt = internal::distance(first, last);

	return status::OK;
}

status radix::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->lower_bound(key);

	cnt = internal::distance(first, last);

	return status::OK;
}

status radix::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (key1.compare(key2) < 0) {
		auto first = container->upper_bound(key1);
		auto last = container->lower_bound(key2);

		cnt = internal::distance(first, last);
	} else {
		cnt = 0;
	}

	return status::OK;
}

int radix::iterate_callback(const container_type::iterator &it, get_kv_callback *callback,
			    void *arg)
{
	const auto &key = it->key();
	const auto &value = it->value();

	return callback(key.data(), key.size(), value.data(), value.size(), arg);
}

status radix::iterate(container_type::iterator first, container_type::iterator last,
		      get_kv_callback *callback, void *arg)
{
	return iterate_generic(
		first,
		[&](const container_type::iterator &it) {
			return iterate_callback(it, callback, arg);
		},
		[&](const container_type::iterator &it) { return it != last; });
}

status radix::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();

	auto first = container->begin();
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status radix::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->upper_bound(key);
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status radix::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->lower_bound(key);
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status radix::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->upper_bound(key);

	return iterate(first, last, callback, arg);
}

status radix::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->lower_bound(key);

	return iterate(first, last, callback, arg);
}

status radix::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (key1.compare(key2) < 0) {
		auto first = container->upper_bound(key1);
		auto last = container->lower_bound(key2);
		return iterate(first, last, callback, arg);
	}

	return status::OK;
}

status radix::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	return container->find(key) != container->end() ? status::OK : status::NOT_FOUND;
}

status radix::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto it = container->find(key);
	if (it != container->end()) {
		auto value = string_view(it->value());
		callback(value.data(), value.size(), arg);
		return status::OK;
	}

	LOG("  key not found");
	return status::NOT_FOUND;
}

status radix::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	auto result = container->try_emplace(key, value);

	if (result.second == false) {
		pmem::obj::transaction::run(pmpool,
					    [&] { result.first.assign_val(value); });
	}

	return status::OK;
}

status radix::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto it = container->find(key);

	if (it == container->end())
		return status::NOT_FOUND;

	container->erase(it);

	return status::OK;
}

internal::transaction *radix::begin_tx()
{
	return new internal::radix::transaction(pmpool, container);
}

void radix::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		auto pmem_ptr = static_cast<pmem_type *>(pmemobj_direct(*root_oid));

		container = &pmem_ptr->map;
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid = pmem::obj::make_persistent<pmem_type>().raw();
			auto pmem_ptr =
				static_cast<pmem_type *>(pmemobj_direct(*root_oid));
			container = &pmem_ptr->map;
		});
	}
}

/* HETEROGENEOUS_RADIX */

static void no_delete(const char *)
{
	/* do nothing */
}

/* Must be called inside EBR critical section. */
heterogeneous_radix::merged_iterator::merged_iterator(
	heterogeneous_radix &hetero_radix,
	typename internal::radix::ordered_cache<uvalue_type>::iterator dram_it,
	typename internal::radix::map_mt_type::iterator pmem_it)
    : hetero_radix(hetero_radix), dram_it(dram_it), pmem_it(pmem_it)
{
	set_current_it();
}

/* Must be called inside EBR critical section. */
heterogeneous_radix::merged_iterator &heterogeneous_radix::merged_iterator::operator++()
{
	assert(dereferenceable());

	if (curr_it == current_it::dram) {
		assert(dram_it != hetero_radix.cache->end());
		assert(pmem_it == hetero_radix.container->end() ||
		       dram_it->first.compare(pmem_it->key()) < 0);
	} else {
		assert(pmem_it != hetero_radix.container->end());
		assert(dram_it == hetero_radix.cache->end() ||
		       dram_it->first.compare(pmem_it->key()) > 0);
	}

	if (curr_it == current_it::dram)
		++dram_it;
	else
		++pmem_it;

	set_current_it();

	return *this;
}

/* Must be called inside EBR critical section. */
string_view heterogeneous_radix::merged_iterator::key() const
{
	assert(dereferenceable());

	if (curr_it == current_it::dram)
		return dram_it->first;
	else
		return pmem_it->key();
}

/* Must be called inside EBR critical section. */
std::pair<heterogeneous_radix::unique_ptr_type, size_t>
heterogeneous_radix::merged_iterator::value() const
{
	assert(dereferenceable());

	if (curr_it == current_it::dram) {
		auto v = &dram_it->second->second;
		auto value = v->load(std::memory_order_acquire);

		assert(value != tombstone_persistent() && value != tombstone_volatile());

		auto size = value->size();
		unique_ptr_type ptr(nullptr, &no_delete);

		while (!ptr)
			ptr = hetero_radix.try_read_value(v, value);

		return std::pair<unique_ptr_type, size_t>(std::move(ptr), size);
	} else {
		const auto &val = pmem_it->value();

		unique_ptr_type ptr(val.data(), no_delete);
		return std::pair<unique_ptr_type, size_t>(std::move(ptr), val.size());
	}
}

bool heterogeneous_radix::merged_iterator::dereferenceable() const
{
	return pmem_it != hetero_radix.container->end() ||
		dram_it != hetero_radix.cache->end();
}

void heterogeneous_radix::merged_iterator::set_current_it()
{
	while (dereferenceable()) {
		if (pmem_it != hetero_radix.container->end() &&
		    dram_it != hetero_radix.cache->end() &&
		    dram_it->first == string_view(pmem_it->key())) {
			/* If keys are the same, skip the one in pmem (the dram one is
			 * more recent) */
			++pmem_it;
		} else if (dram_it == hetero_radix.cache->end() ||
			   (pmem_it != hetero_radix.container->end() &&
			    dram_it->first.compare(pmem_it->key()) > 0)) {
			/* If there are no more dram elements or pmem element is smaller
			 */
			curr_it = current_it::pmem;
			return;
		} else {
			auto v = dram_it->second->second.load(std::memory_order_acquire);

			/* Skip removed entries */
			if (v == heterogeneous_radix::tombstone_volatile() ||
			    v == heterogeneous_radix::tombstone_persistent()) {
				++dram_it;
			} else {
				curr_it = current_it::dram;
				return;
			}
		}
	}
}

const heterogeneous_radix::uvalue_type *heterogeneous_radix::tombstone_volatile()
{
	return reinterpret_cast<const heterogeneous_radix::uvalue_type *>(1ULL);
}

const heterogeneous_radix::uvalue_type *heterogeneous_radix::tombstone_persistent()
{
	return reinterpret_cast<const heterogeneous_radix::uvalue_type *>(2ULL);
}

heterogeneous_radix::queue_entry::queue_entry(cache_type::value_type *dram_entry,
					      string_view key_, string_view value_)
    : dram_entry(dram_entry)
{
	auto key_size = pmem::obj::experimental::total_sizeof<uvalue_type>::value(key_);
	auto key_dst = reinterpret_cast<uvalue_type *>(this + 1);
	auto padding = align_up(key_size, alignof(uvalue_type)) - key_size;
	auto val_dst = reinterpret_cast<uvalue_type *>(reinterpret_cast<char *>(key_dst) +
						       key_size + padding);

	new (key_dst) uvalue_type(key_);

	if (value_.data() == nullptr) {
		new (val_dst) uvalue_type(string_view(""));
		remove = true;
	} else {
		new (val_dst) uvalue_type(value_);
		remove = false;
	}
}

const heterogeneous_radix::uvalue_type &heterogeneous_radix::queue_entry::key() const
{
	auto key = reinterpret_cast<const uvalue_type *>(this + 1);
	return *key;
}

const heterogeneous_radix::uvalue_type &heterogeneous_radix::queue_entry::value() const
{
	auto key_size = pmem::obj::experimental::total_sizeof<uvalue_type>::value(
		string_view(key()));
	auto key_dst = reinterpret_cast<const char *>(this + 1);
	auto padding = align_up(key_size, alignof(uvalue_type)) - key_size;
	auto val_dst =
		reinterpret_cast<const uvalue_type *>(key_dst + key_size + padding);

	return *reinterpret_cast<const uvalue_type *>(val_dst);
}

heterogeneous_radix::uvalue_type &heterogeneous_radix::queue_entry::value()
{
	auto val = &const_cast<const queue_entry *>(this)->value();
	return *const_cast<heterogeneous_radix::uvalue_type *>(val);
}

heterogeneous_radix::heterogeneous_radix(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg, "pmemkv_radix"), config(std::move(cfg))
{
	config->get_uint64("log_size", &log_size);
	config->get_uint64("cache_size", &cache_size);

	pmem_type *pmem_ptr;

	if (!OID_IS_NULL(*root_oid)) {
		pmem_ptr = static_cast<pmem_type *>(pmemobj_direct(*root_oid));
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid = pmem::obj::make_persistent<pmem_type>().raw();
			pmem_ptr = static_cast<pmem_type *>(pmemobj_direct(*root_oid));
			pmem_ptr->log =
				pmem::obj::make_persistent<internal::radix::log_type>(
					log_size);
		});
	}

	log = pmem_ptr->log.get();

	container = &pmem_ptr->map;
	container->runtime_initialize_mt();
	container_worker = std::unique_ptr<container_type::ebr::worker>(
		new container_type::ebr::worker(container->register_worker()));

	cache = std::unique_ptr<cache_type>(new cache_type(cache_size));
	queue = std::unique_ptr<pmem_queue_type>(new pmem_queue_type(*pmem_ptr->log, 1));
	queue_worker = std::unique_ptr<pmem_queue_type::worker>(
		new pmem_queue_type::worker(queue->register_worker()));

	queue->try_consume_batch([&](pmem_queue_type::batch_type batch) {
		for (auto entry : batch)
			consume_queue_entry(entry, false);
	});

	bg_exception_ptr = nullptr;

	stopped.store(false);
	bg_thread = std::thread([&] { bg_work(); });

	pop = pmem::obj::pool_by_vptr(&pmem_ptr->log);
}

heterogeneous_radix::~heterogeneous_radix()
{
	{
		std::unique_lock<std::mutex> lock(bg_lock);
		stopped.store(true);
	}

	bg_cv.notify_one();

	bg_thread.join();

	container_worker.reset(nullptr);
	queue_worker.reset(nullptr);

	container->runtime_finalize_mt();
}

bool heterogeneous_radix::log_contains(const void *ptr) const
{
	auto begin = log->data().data();
	auto size = log->data().size();

	return (begin <= reinterpret_cast<const char *>(ptr)) &&
		(begin + size >= reinterpret_cast<const char *>(ptr));
}

heterogeneous_radix::cache_type::value_type *
heterogeneous_radix::cache_put_with_evict(string_view key, const uvalue_type *value)
{
	auto evict_cb = [&](typename cache_type::lru_list_type &list) {
		for (auto rit = list.rbegin(); rit != list.rend(); rit++) {
			auto t = rit->second.load(std::memory_order_relaxed);

			/* Only element which has already been process by background
			 * thread (is not in the log) can be evicted. */
			if (!log_contains(t) && t != tombstone_volatile())
				return std::next(rit).base();
		}

		return list.end();
	};

	return cache->put(key, value, evict_cb);
}

void heterogeneous_radix::handle_oom_from_bg()
{
	std::exception_ptr *exc;
	if ((exc = bg_exception_ptr.load(std::memory_order_acquire)) != nullptr) {
		std::rethrow_exception(*exc);
	}
}

status heterogeneous_radix::put(string_view key, string_view value)
{
	check_outside_tx();

	/*
	 * This implementation consists of following steps:
	 * 1. Insert element to the DRAM cache (with empty value for now)
	 * 2. Allocate queue_entry on dram (it will hold key/value or
	 * key/tombstone pair).
	 * 3. Try to produce the queue_entry using queue. If this succeeds
	 * set cache entry value to point to the value in queue.
	 *
	 * If inserting to cache or producing the queue_entry fails, check
	 * if background thread did not encounter oom. If yes, propagate
	 * oom to the user.
	 */

	auto uvalue_key_size =
		pmem::obj::experimental::total_sizeof<uvalue_type>::value(key);
	auto padding = align_up(uvalue_key_size, alignof(uvalue_type)) - uvalue_key_size;
	auto req_size = uvalue_key_size +
		pmem::obj::experimental::total_sizeof<uvalue_type>::value(value) +
		sizeof(queue_entry) + padding;

	using alloc_type = typename std::aligned_storage<sizeof(queue_entry),
							 alignof(queue_entry)>::type;
	auto alloc_size = (req_size + sizeof(queue_entry) - 1) / sizeof(queue_entry);
	auto data = std::unique_ptr<alloc_type[]>(new alloc_type[alloc_size]);

	assert(reinterpret_cast<uintptr_t>(data.get()) % alignof(queue_entry) == 0);

	/* XXX: implement blocking cache_put_with_evict */
	cache_type::value_type *cache_val = nullptr;
	while (cache_val == nullptr) {
		cache_val = cache_put_with_evict(key, nullptr);
		handle_oom_from_bg();
	}

	new (data.get()) queue_entry(cache_val, key, value);

	while (true) {
		auto produced = queue_worker->try_produce(
			pmem::obj::string_view(reinterpret_cast<const char *>(data.get()),
					       req_size),
			[&](pmem::obj::string_view target) {
				auto pmem_entry = reinterpret_cast<const queue_entry *>(
					target.data());
				assert(pmem_entry->key() == key);

				const uvalue_type *val =
					(!value.data() ? tombstone_volatile()
						       : &pmem_entry->value());

				cache_val->store(val, std::memory_order_release);
			});
		if (produced)
			break;
		else
			handle_oom_from_bg();
	}

	// XXX - if try_produce == false, we can just allocate new radix node to
	// TLS and the publish pointer to this node
	// NEED TX support for produce():
	// tx {queue.produce([&] (data) { data = make_persistent(); }) }

	return status::OK;
}

status heterogeneous_radix::remove(string_view k)
{
	check_outside_tx();

	/* Check if element exists. */
	bool found = false;
	auto v = cache->get(k, false);
	if (v) {
		auto value = v->load(std::memory_order_acquire);
		found = (value != tombstone_persistent() &&
			 value != tombstone_volatile());
	} else {
		found = container->find(k) != container->end();
	}

	if (!found)
		return status::NOT_FOUND;

	try {
		/* Put tombstone. */
		auto s = put(k, pmem::obj::string_view());

		if (s != status::OK)
			return s;
	} catch (pmem::transaction_out_of_memory &) {
		/* Set element in cache to tombstone, does nothing if element
		 * is not in the cache. */
		cache->put(k, tombstone_persistent(),
			   [&](typename cache_type::lru_list_type &list) {
				   return list.end();
			   });

		/* Try to free the element directly, bypassing the queue. No
		 * synchronization is needed with bg thread since it is blocked by OOM. */
		assert(bg_exception_ptr.load(std::memory_order_relaxed));

		container->erase(k);
		container->garbage_collect_force();

		delete bg_exception_ptr.load(std::memory_order_relaxed);

		{
			std::unique_lock<std::mutex> lock(bg_lock);
			bg_exception_ptr.store(nullptr, std::memory_order_release);
		}

		/* Notify bg thread that the exception was consumed */
		bg_cv.notify_one();
	}

	return status::OK;
}

/* Tries to optimistically read from the log. Operation succeeds only
 * if v->load() points equals value after read completes. Otherwise nullptr
 * is returned and value is set to v->load() */
heterogeneous_radix::unique_ptr_type
heterogeneous_radix::try_read_value(cache_type::value_type *v,
				    const uvalue_type *&value) const
{
	auto size = value->size();
	auto data = value->data();

	if (log_contains(value) && log_contains(data + size)) {
		return log_read_optimistically(v, value);
	} else {
		/* Cache entry points to data in pmem container. It's safe
		 * to just read from it as entries in containers are
		 * protected by EBR. */
		auto ptr = unique_ptr_type(value->data(), &no_delete);
		assert(v->load(std::memory_order_acquire) == value);

		return ptr;
	}
}

heterogeneous_radix::unique_ptr_type
heterogeneous_radix::log_read_optimistically(cache_type::value_type *v,
					     const uvalue_type *&value) const
{
	auto data = value->data();
	auto size = value->size();

	/* Cache entry points to data in log. To read the data we
	 * must protect against producers which could overwrite
	 * the data concurrently. To do this we use variant of
	 * optimistic concurrency control: we validate that data
	 * is entirely inside the log and copy the data to
	 * temporary buffer. After that, we check if cache entry
	 * changed. If it did, it means that the entry was
	 * conusmed by bg thread and producers might have
	 * overwritten the data - in this case we start from the
	 * beginning. Otherwise we just call user callback with
	 * the temporary buffer. */
	auto unsafe_buff = new char[size];
	std::copy(data, data + size, unsafe_buff);

	auto buffer = unique_ptr_type(unsafe_buff, [](const char *p) { delete[] p; });

	auto current_value = v->load(std::memory_order_acquire);
	if (current_value == value) {
		return buffer;
	} else {
		value = current_value;
		return unique_ptr_type(nullptr, &no_delete);
	}
}

status heterogeneous_radix::get(string_view key, get_v_callback *callback, void *arg)
{
	check_outside_tx();
	status s = status::OK;

	auto v = cache->get(key, true);

	container_worker->critical([&] {
		if (!v) {
			/* If element is not in the cache, search radix tree. */
			auto it = container->find(key);
			if (it != container->end()) {
				auto value = string_view(it->value());
				callback(value.data(), value.size(), arg);

				cache_put_with_evict(key, &it->value());

				s = status::OK;
			} else
				s = status::NOT_FOUND;

			return;
		}

		unique_ptr_type ptr = unique_ptr_type(nullptr, &no_delete);
		size_t size;
		while (!ptr) {
			auto value = v->load(std::memory_order_acquire);
			if (value == tombstone_volatile() ||
			    value == tombstone_persistent()) {
				s = status::NOT_FOUND;
				return;
			}

			size = value->size();
			ptr = try_read_value(v, value);
		}

		callback(ptr.get(), size, arg);
		s = status::OK;
	});

	return s;
}

std::string heterogeneous_radix::name()
{
	return "radix";
}

heterogeneous_radix::merged_iterator heterogeneous_radix::merged_begin()
{
	return merged_iterator(*this, cache->begin(), container->begin());
}

heterogeneous_radix::merged_iterator heterogeneous_radix::merged_end()
{
	return merged_iterator(*this, cache->end(), container->end());
}

heterogeneous_radix::merged_iterator
heterogeneous_radix::merged_lower_bound(string_view key)
{
	auto dram_lo = cache->lower_bound(key);
	auto pmem_lo = container->lower_bound(key);

	assert(dram_lo == cache->end() || dram_lo->first.compare(key) >= 0);
	assert(pmem_lo == container->end() || pmem_lo->key().compare(key) >= 0);

	return merged_iterator(*this, dram_lo, pmem_lo);
}

heterogeneous_radix::merged_iterator
heterogeneous_radix::merged_upper_bound(string_view key)
{
	auto dram_up = cache->upper_bound(key);
	auto pmem_up = container->upper_bound(key);

	assert(dram_up == cache->end() || dram_up->first.compare(key) > 0);
	assert(pmem_up == container->end() || pmem_up->key().compare(key) > 0);

	return merged_iterator(*this, dram_up, pmem_up);
}

int heterogeneous_radix::iterate_callback(const merged_iterator &it,
					  get_kv_callback *callback, void *arg)
{
	const auto &key = it.key();
	auto val = it.value();

	return callback(key.data(), key.size(), val.first.get(), val.second, arg);
}

status heterogeneous_radix::get_all(get_kv_callback *callback, void *arg)
{
	check_outside_tx();

	status s;
	container_worker->critical([&] {
		auto first = merged_begin();

		s = iterate_generic(
			first,
			[&](const merged_iterator &it) {
				return iterate_callback(it, callback, arg);
			},
			[&](const merged_iterator &it) { return it.dereferenceable(); });
	});

	return s;
}

status heterogeneous_radix::get_above(string_view key, get_kv_callback *callback,
				      void *arg)
{
	check_outside_tx();

	status s;
	container_worker->critical([&] {
		auto first = merged_upper_bound(key);

		s = iterate_generic(
			first,
			[&](const merged_iterator &it) {
				return iterate_callback(it, callback, arg);
			},
			[&](const merged_iterator &it) { return it.dereferenceable(); });
	});

	return s;
}

status heterogeneous_radix::get_equal_above(string_view key, get_kv_callback *callback,
					    void *arg)
{
	check_outside_tx();

	status s;
	container_worker->critical([&] {
		auto first = merged_lower_bound(key);

		s = iterate_generic(
			first,
			[&](const merged_iterator &it) {
				return iterate_callback(it, callback, arg);
			},
			[&](const merged_iterator &it) { return it.dereferenceable(); });
	});

	return s;
}

status heterogeneous_radix::get_equal_below(string_view key, get_kv_callback *callback,
					    void *arg)
{
	check_outside_tx();

	status s;
	container_worker->critical([&] {
		auto first = merged_begin();

		/* We cannot rely on iterator comparisons because of concurrent
		 * inserts/erases. There are two problems:
		 * 1. Iterator can be erased. In that case `while(it != last) it++;` would
		 *    spin forever.
		 * 2. `auto last = merged_upper_bound(key);
		 *     while (it != last) it++;`
		 *
		 *     if merged_upper_bound(key) returns end(), this means there are no
		 *     elements bigger than key - we can then return all elements
		 *     from the container, right?
		 *
		 *     Not really - while we iterate someone might insert element bigger
		 * than key and we will iterate over such element (since we are processing
		 * all elements).
		 */
		s = iterate_generic(
			first,
			[&](const merged_iterator &it) {
				return iterate_callback(it, callback, arg);
			},
			[&](const merged_iterator &it) {
				return it.dereferenceable() && it.key().compare(key) <= 0;
			});
	});

	return s;
}

status heterogeneous_radix::get_below(string_view key, get_kv_callback *callback,
				      void *arg)
{
	check_outside_tx();

	status s;
	container_worker->critical([&] {
		auto first = merged_begin();

		s = iterate_generic(
			first,
			[&](const merged_iterator &it) {
				return iterate_callback(it, callback, arg);
			},
			[&](const merged_iterator &it) {
				return it.dereferenceable() && it.key().compare(key) < 0;
			});
	});

	return s;
}

status heterogeneous_radix::get_between(string_view key1, string_view key2,
					get_kv_callback *callback, void *arg)
{
	check_outside_tx();

	if (key1.compare(key2) < 0) {
		status s;
		container_worker->critical([&] {
			auto first = merged_upper_bound(key1);

			s = iterate_generic(
				first,
				[&](const merged_iterator &it) {
					return iterate_callback(it, callback, arg);
				},
				[&](const merged_iterator &it) {
					return it.dereferenceable() &&
						it.key().compare(key2) < 0;
				});
		});

		return s;
	}

	return status::OK;
}

/* Used as a callback for get_* methods, increments a counter passed through @param arg on
 * each call. */
static int count_elements(const char *, size_t, const char *, size_t, void *arg)
{
	auto *cnt = static_cast<size_t *>(arg);
	++(*cnt);

	return 0;
}

status heterogeneous_radix::count_all(std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_all(count_elements, (void *)&cnt);
}

status heterogeneous_radix::count_above(string_view key, std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_above(key, count_elements, (void *)&cnt);
}

status heterogeneous_radix::count_equal_above(string_view key, std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_equal_above(key, count_elements, (void *)&cnt);
}

status heterogeneous_radix::count_equal_below(string_view key, std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_equal_below(key, count_elements, (void *)&cnt);
}

status heterogeneous_radix::count_below(string_view key, std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_below(key, count_elements, (void *)&cnt);
}

status heterogeneous_radix::count_between(string_view key1, string_view key2,
					  std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_between(key1, key2, count_elements, (void *)&cnt);
}

status heterogeneous_radix::exists(string_view key)
{
	return get(
		key, [](const char *, size_t, void *) {}, nullptr);
}

void heterogeneous_radix::consume_queue_entry(pmem::obj::string_view entry,
					      bool dram_is_valid)
{
	/*
	 * This function consumes entries for the queue. It inserts/erases the
	 * element from the radix tree.
	 *
	 * If dram_is_valid is set it also exchanges pointer in dram_entry so
	 * that it now points to radix_tree node (or tombstone for erase).
	 * This can only be done if dram_entry was allocated in this application
	 * run (so, only from bg_work()).
	 *
	 * This allows us to keep processed elements in cache without additional
	 * value copies.
	 */
	auto e = reinterpret_cast<const queue_entry *>(entry.data());
	auto dram_entry = const_cast<queue_entry *>(e)->dram_entry;
	const uvalue_type *expected = e->remove ? tombstone_volatile() : &e->value();
	const uvalue_type *desired;

	/* If the dram_entry points to different element than was passed through queue
	 * it is already outdated - just skip it, it will be handled later. */
	if (dram_is_valid && dram_entry->load(std::memory_order_acquire) != expected)
		return;

	if (e->remove) {
		container->erase(e->key());
		desired = tombstone_persistent();
	} else {
		auto ret = container->insert_or_assign(e->key(), e->value());
		desired = &ret.first->value();
	}

	if (dram_is_valid)
		dram_entry->compare_exchange_strong(expected, desired);
}

void heterogeneous_radix::bg_work()
{
	bool should_report_oom = false;
	while (true) {
		/* XXX: only stop if all elements are consumed ? */
		if (stopped.load())
			return;

		try {
			auto consumed = queue->try_consume_batch(
				[&](pmem_queue_type::batch_type batch) {
					for (auto entry : batch)
						consume_queue_entry(entry, true);
				});

			if (consumed) {
				should_report_oom = false;
			} else {
				/* Nothing else to do, try to collect some
				 * garbage. */
				container->garbage_collect();
			}
		} catch (...) {
			if (!should_report_oom) {
				/* Try to force clear garbage. If that fails, we will
				 * report oom for the user in next iteration. */
				try {
					should_report_oom = true;
					container->garbage_collect_force();
					continue;
				} catch (...) {
				}
			}

			auto ex = new std::exception_ptr(std::current_exception());
			bg_exception_ptr.store(ex);

			std::unique_lock<std::mutex> lock(bg_lock);

			/* Wait until exception is handled or
			 * engine is stopped. */
			bg_cv.wait(lock, [&] {
				return bg_exception_ptr.load() == nullptr ||
					stopped.load();
			});

			/* If engine is stopped, abort consume */
			if (stopped.load())
				return;

			should_report_oom = false;
		}
	}
}

internal::iterator_base *radix::new_iterator()
{
	return new radix_iterator<false>{container};
}

internal::iterator_base *radix::new_const_iterator()
{
	return new radix_iterator<true>{container};
}

radix::radix_iterator<true>::radix_iterator(container_type *c)
    : container(c), pop(pmem::obj::pool_by_vptr(c))
{
}

radix::radix_iterator<false>::radix_iterator(container_type *c)
    : radix::radix_iterator<true>(c)
{
}

status radix::radix_iterator<true>::seek(string_view key)
{
	init_seek();

	it_ = container->find(key);
	if (it_ != container->end())
		return status::OK;

	return status::NOT_FOUND;
}

status radix::radix_iterator<true>::seek_lower(string_view key)
{
	init_seek();

	it_ = container->lower_bound(key);
	if (it_ == container->begin()) {
		it_ = container->end();
		return status::NOT_FOUND;
	}

	--it_;

	return status::OK;
}

status radix::radix_iterator<true>::seek_lower_eq(string_view key)
{
	init_seek();

	it_ = container->upper_bound(key);
	if (it_ == container->begin()) {
		it_ = container->end();
		return status::NOT_FOUND;
	}

	--it_;

	return status::OK;
}

status radix::radix_iterator<true>::seek_higher(string_view key)
{
	init_seek();

	it_ = container->upper_bound(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::seek_higher_eq(string_view key)
{
	init_seek();

	it_ = container->lower_bound(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::seek_to_first()
{
	init_seek();

	if (container->empty())
		return status::NOT_FOUND;

	it_ = container->begin();

	return status::OK;
}

status radix::radix_iterator<true>::seek_to_last()
{
	init_seek();

	if (container->empty())
		return status::NOT_FOUND;

	it_ = container->end();
	--it_;

	return status::OK;
}

status radix::radix_iterator<true>::is_next()
{
	auto tmp = it_;
	if (tmp == container->end() || ++tmp == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::next()
{
	init_seek();

	if (it_ == container->end() || ++it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::prev()
{
	init_seek();

	if (it_ == container->begin())
		return status::NOT_FOUND;

	--it_;

	return status::OK;
}

result<string_view> radix::radix_iterator<true>::key()
{
	assert(it_ != container->end());

	return string_view(it_->key().cdata(), it_->key().size());
}

result<pmem::obj::slice<const char *>> radix::radix_iterator<true>::read_range(size_t pos,
									       size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->value().size() || pos + n < pos)
		n = it_->value().size() - pos;

	return {{it_->value().cdata() + pos, it_->value().cdata() + pos + n}};
}

result<pmem::obj::slice<char *>> radix::radix_iterator<false>::write_range(size_t pos,
									   size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->value().size() || pos + n < pos)
		n = it_->value().size() - pos;

	log.push_back({std::string(it_->value().cdata() + pos, n), pos});
	auto &val = log.back().first;

	return {{&val[0], &val[n]}};
}

status radix::radix_iterator<false>::commit()
{
	pmem::obj::transaction::run(pop, [&] {
		for (auto &p : log) {
			auto dest = it_->value().range(p.second, p.first.size());
			std::copy(p.first.begin(), p.first.end(), dest.begin());
		}
	});
	log.clear();

	return status::OK;
}

void radix::radix_iterator<false>::abort()
{
	log.clear();
}

static factory_registerer
	register_radix(std::unique_ptr<engine_base::factory_base>(new radix_factory));

} // namespace kv
} // namespace pmem
