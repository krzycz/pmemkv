// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#ifndef LIBPMEMKV_PMEMOBJ_ENGINE_H
#define LIBPMEMKV_PMEMOBJ_ENGINE_H

#include <iostream>
#include <unistd.h>

#include "engine.h"
#include "libpmemkv.h"
#include <libpmemobj++/pool.hpp>

namespace pmem
{

/* Helper method which throws an exception when called in a tx */
static inline void check_outside_tx()
{
	if (pmemobj_tx_stage() != TX_STAGE_NONE)
		throw transaction_scope_error(
			"Function called inside transaction scope.");
}

namespace kv
{

template <typename EngineData>
class pmemobj_engine_base : public engine_base {
public:
	pmemobj_engine_base(std::unique_ptr<internal::config> &cfg,
			    const std::string &layout)
	{
		const char *path = nullptr;
		std::size_t size;
		PMEMoid *oid;

		auto is_path = cfg->get_string("path", &path);
		auto is_oid = cfg->get_object("oid", (void **)&oid);

		if (is_path && is_oid) {
			throw internal::invalid_argument(
				"Config contains both: \"path\" and \"oid\"");
		} else if (!is_path && !is_oid) {
			throw internal::invalid_argument(
				"Config does not contain item with key: \"path\" or \"oid\"");
		} else if (is_path) {
			uint64_t force_create;
			cfg_by_path = true;

			if (!cfg->get_uint64("force_create", &force_create)) {
				force_create = 0;
			}

			pmem::obj::pool<Root> pop;
			if (force_create) {
				if (!cfg->get_uint64("size", &size))
					throw internal::invalid_argument(
						"Config does not contain item with key: \"size\"");

				pop = pmem::obj::pool<Root>::create(path, layout, size,
								    S_IRWXU);
			} else {
				pop = pmem::obj::pool<Root>::open(path, layout);
			}

			root_oid = pop.root()->ptr.raw_ptr();
			pmpool = pop;

		} else if (is_oid) {
			pmpool = pmem::obj::pool_base(pmemobj_pool_by_ptr(oid));
			root_oid = oid;
		}

		int64_t stats = 0;

		if (cfg->get_int64("pmemobj_stats_enabled", &stats)) {
			switch (stats)  {
				case 0:
					stats = POBJ_STATS_ENABLED_TRANSIENT;
					break;
				case 1:
					stats = POBJ_STATS_ENABLED_BOTH;
					break;
				case 2:
					stats = POBJ_STATS_ENABLED_PERSISTENT;
					break;
				case 3:
					stats = POBJ_STATS_DISABLED;
					break;
				default:
					throw pmem::kv::internal::invalid_argument(
						"Unsupported value for \"pmemobj_stats_enabled\" config parameter.");
			}

			if (0 != pmemobj_ctl_set(pmpool.handle(), "stats.enabled", &stats))
				throw internal::error("Failed to set \"pmemobj_stats_enabled\".");
		}
	}

	~pmemobj_engine_base()
	{
		if (cfg_by_path)
			pmpool.close();
	}

	virtual status stats(statistics *stat) {
		int ret = 0;

		ret = pmemobj_ctl_get(pmpool.handle(), "stats.enabled", &stat->stats_enabled);
		if (ret != 0) {
			out_err_stream("stats") << "Failed to query \"stats.enabled\".";
			return status::STATS_ERROR;
		}

		ret = pmemobj_ctl_get(pmpool.handle(), "stats.heap.curr_allocated", &stat->heap_curr_allocated);
		if (ret != 0) {
			out_err_stream("stats") << "Failed to query \"stats.heap.curr_allocated\".";
			return status::STATS_ERROR;
		}

		ret = pmemobj_ctl_get(pmpool.handle(), "stats.heap.run_allocated", &stat->heap_run_allocated);
		if (ret != 0) {
			out_err_stream("stats") << "Failed to query \"stats.heap.run_allocated\".";
			return status::STATS_ERROR;
		}

		ret = pmemobj_ctl_get(pmpool.handle(), "stats.heap.run_active", &stat->heap_run_active);
		if (ret != 0) {
			out_err_stream("stats") << "Failed to query \"stats.heap.run_active\".";
			return status::STATS_ERROR;
		}

		return status::OK;
	};

protected:
	struct Root {
		pmem::obj::persistent_ptr<EngineData>
			ptr; /* used when path is specified */
	};

	pmem::obj::pool_base pmpool;
	PMEMoid *root_oid;

	bool cfg_by_path = false;
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_PMEMOBJ_ENGINE_H */
