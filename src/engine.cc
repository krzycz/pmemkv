// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "engine.h"

#include "engines/blackhole.h"

#ifdef ENGINE_VSMAP
#include "engines/vsmap.h"
#endif

#ifdef ENGINE_VCMAP
#include "engines/vcmap.h"
#endif

#ifdef ENGINE_CMAP
#include "engines/cmap.h"
#endif

#ifdef ENGINE_CSMAP
#include "engines-experimental/csmap.h"
#endif

#ifdef ENGINE_RADIX
#include "engines-experimental/radix.h"
#endif

#ifdef ENGINE_STREE
#include "engines-experimental/stree.h"
#endif

#ifdef ENGINE_TREE3
#include "engines-experimental/tree3.h"
#endif

#ifdef ENGINE_ROBINHOOD
#include "engines-experimental/robinhood.h"
#endif

#ifdef ENGINE_DRAM_VCMAP
#include "engines-testing/dram_vcmap.h"
#endif

#ifdef ENGINE_DRAM_VCSMAP
#include "engines-testing/dram_vcsmap.h"
#endif

namespace pmem
{
namespace kv
{

engine_base::engine_base()
{
}

engine_base::~engine_base()
{
}

static constexpr const char *available_engines = "blackhole"
#ifdef ENGINE_CMAP
						 ", cmap"
#endif
#ifdef ENGINE_CSMAP
						 ", csmap"
#endif
#ifdef ENGINE_RADIX
						 ", radix"
#endif
#ifdef ENGINE_VSMAP
						 ", vsmap"
#endif
#ifdef ENGINE_VCMAP
						 ", vcmap"
#endif
#ifdef ENGINE_TREE3
						 ", tree3"
#endif
#ifdef ENGINE_STREE
						 ", stree"
#endif
#ifdef ENGINE_ROBINHOOD
						 ", robinhood"
#endif
#ifdef ENGINE_DRAM_VCMAP
						 ", dram_vcmap"
#endif
#ifdef ENGINE_DRAM_VCSMAP
						 ", dram_vcsmap"
#endif
	;

/* throws internal error (status) if config is null */
void engine_base::check_config_null(const std::string &engine_name,
				    std::unique_ptr<internal::config> &cfg)
{
	if (!cfg) {
		throw internal::invalid_argument("Config cannot be null for the '" +
						 engine_name + "' engine");
	}
}

std::unique_ptr<engine_base>
engine_base::create_engine(const std::string &engine,
			   std::unique_ptr<internal::config> cfg)
{
	if (engine == "blackhole")
		return std::unique_ptr<engine_base>(
			new pmem::kv::blackhole(std::move(cfg)));

#ifdef ENGINE_CMAP
	if (engine == "cmap") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::cmap(std::move(cfg)));
	}
#endif

#ifdef ENGINE_CSMAP
	if (engine == "csmap") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::csmap(std::move(cfg)));
	}
#endif

#ifdef ENGINE_RADIX
	if (engine == "radix") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::radix(std::move(cfg)));
	}
#endif

#ifdef ENGINE_VSMAP
	if (engine == "vsmap") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::vsmap(std::move(cfg)));
	}
#endif

#ifdef ENGINE_VCMAP
	if (engine == "vcmap") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::vcmap(std::move(cfg)));
	}
#endif

#ifdef ENGINE_TREE3
	if (engine == "tree3") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::tree3(std::move(cfg)));
	}
#endif

#ifdef ENGINE_STREE
	if (engine == "stree") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::stree(std::move(cfg)));
	}
#endif

#ifdef ENGINE_ROBINHOOD
	if (engine == "robinhood") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(
			new pmem::kv::robinhood(std::move(cfg)));
	}
#endif

#ifdef ENGINE_DRAM_VCMAP
	if (engine == "dram_vcmap") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(
			new pmem::kv::dram_vcmap(std::move(cfg)));
	}
#endif

#ifdef ENGINE_DRAM_VCSMAP
	if (engine == "dram_vcsmap") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(
			new pmem::kv::dram_vcsmap(std::move(cfg)));
	}
#endif

	throw internal::wrong_engine_name("Unknown engine name \"" + engine +
					  "\". Available engines: " + available_engines);
}

status engine_base::count_all(std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_above(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_equal_above(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_equal_below(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_below(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}
status engine_base::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_all(get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_between(string_view key1, string_view key2,
				get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::exists(string_view key)
{
	return status::NOT_SUPPORTED;
}

status engine_base::defrag(double start_percent, double amount_percent)
{
	return status::NOT_SUPPORTED;
}

internal::transaction *engine_base::begin_tx()
{
	throw internal::not_supported("Transactions are not supported in this engine");
}

engine_base::iterator *engine_base::new_iterator()
{
	throw internal::not_supported("Iterators are not supported in this engine");
}

engine_base::iterator *engine_base::new_const_iterator()
{
	throw internal::not_supported("Iterators are not supported in this engine");
}

} // namespace kv
} // namespace pmem
