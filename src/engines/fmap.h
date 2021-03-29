// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#pragma once

#include <cstdlib>
#include <memory>
#include <vector>

#include "AepManager.hpp"
#include "Utils.hpp"
#include "db.hpp"

#include <iostream>
#include <unistd.h>
#include "../engine.h"
#include "libpmemkv.h"

namespace pmem
{
namespace kv
{
namespace internal
{
namespace fmap
{

} /* namespace fmap */
} /* namespace internal */

class fmap : public engine_base {
public:
	//static Status CreateOrOpen(const std::string &name, DB **dbptr);

	fmap(std::unique_ptr<internal::config> cfg);

	~fmap();

	std::string name() final;

	void Init(const std::string &name);

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

private:
	bool cfg_by_path = false;
	
	AepManager aep_;
	std::string file_name_;

	char *pmem_base_;
	size_t mapped_len_;
	int is_pmem_;
};

} /* namespace kv */
} /* namespace pmem */
