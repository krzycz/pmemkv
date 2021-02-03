// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "cmap.h"
#include "../out.h"

#include <unistd.h>

namespace pmem
{
namespace kv
{

cmap::cmap(std::unique_ptr<internal::config> cfg) : pmemobj_engine_base(cfg, "pmemkv")
{
	static_assert(
		sizeof(internal::cmap::string_t) == 40,
		"Wrong size of cmap value and key. This probably means that std::string has size > 32");

	/* try to read reserve count from config, 0 means nothing will be reserved */
	uint64_t reserve_count = 0;
	cfg->get_uint64("reserve", &reserve_count);

	Recover(reserve_count);
	LOG("Started ok");
}

cmap::~cmap()
{
	LOG("Stopped ok");
}

std::string cmap::name()
{
	return "cmap";
}

status cmap::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();
	cnt = container->size();

	return status::OK;
}

status cmap::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();
	for (auto it = container->begin(); it != container->end(); ++it) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status cmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	return container->count(key) == 1 ? status::OK : status::NOT_FOUND;
}

status cmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	internal::cmap::map_t::const_accessor result;
	bool found = container->find(result, key);
	if (!found) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	callback(result->second.c_str(), result->second.size(), arg);
	return status::OK;
}

status cmap::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	container->insert_or_assign(key, value);

	return status::OK;
}

status cmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	bool erased = container->erase(key);
	return erased ? status::OK : status::NOT_FOUND;
}

status cmap::defrag(double start_percent, double amount_percent)
{
	LOG("defrag: start_percent = " << start_percent
				       << " amount_percent = " << amount_percent);
	check_outside_tx();

	try {
		container->defragment(start_percent, amount_percent);
	} catch (std::range_error &e) {
		out_err_stream("defrag") << e.what();
		return status::INVALID_ARGUMENT;
	} catch (pmem::defrag_error &e) {
		out_err_stream("defrag") << e.what();
		return status::DEFRAG_ERROR;
	}

	return status::OK;
}

void cmap::Recover(size_t reserve_count = 0)
{
	if (!OID_IS_NULL(*root_oid)) {
		container = (pmem::kv::internal::cmap::map_t *)pmemobj_direct(*root_oid);
		container->runtime_initialize();
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid =
				pmem::obj::make_persistent<internal::cmap::map_t>().raw();
			container = (pmem::kv::internal::cmap::map_t *)pmemobj_direct(
				*root_oid);
			container->runtime_initialize();
		});
	}

	/* TODO: change to reserve when it's public */
	container->rehash(reserve_count);
}

internal::iterator_base *cmap::new_iterator()
{
	return new cmap_iterator<false>{container};
}

internal::iterator_base *cmap::new_const_iterator()
{
	return new cmap_iterator<true>{container};
}

cmap::cmap_iterator<true>::cmap_iterator(container_type *c)
    : container(c), pop(pmem::obj::pool_by_vptr(c))
{
}

cmap::cmap_iterator<false>::cmap_iterator(container_type *c)
    : cmap::cmap_iterator<true>(c)
{
}

status cmap::cmap_iterator<true>::seek(string_view key)
{
	init_seek();

	if (container->find(acc_, key))
		return status::OK;

	return status::NOT_FOUND;
}

result<string_view> cmap::cmap_iterator<true>::key()
{
	assert(!acc_.empty());

	return {{acc_->first.c_str()}};
}

result<pmem::obj::slice<const char *>> cmap::cmap_iterator<true>::read_range(size_t pos,
									     size_t n)
{
	assert(!acc_.empty());

	if (pos + n > acc_->second.size() || pos + n < pos)
		n = acc_->second.size() - pos;

	return {{acc_->second.c_str() + pos, acc_->second.c_str() + pos + n}};
}

result<pmem::obj::slice<char *>> cmap::cmap_iterator<false>::write_range(size_t pos,
									 size_t n)
{
	assert(!acc_.empty());

	if (pos + n > acc_->second.size() || pos + n < pos)
		n = acc_->second.size() - pos;

	log.push_back({std::string(acc_->second.c_str() + pos, n), pos});
	auto &val = log.back().first;

	return {{&val[0], &val[n]}};
}

status cmap::cmap_iterator<false>::commit()
{
	pmem::obj::transaction::run(pop, [&] {
		for (auto &p : log) {
			auto dest = acc_->second.range(p.second, p.first.size());
			std::copy(p.first.begin(), p.first.end(), dest.begin());
		}
	});
	log.clear();

	return status::OK;
}

void cmap::cmap_iterator<false>::abort()
{
	log.clear();
}

} // namespace kv
} // namespace pmem
