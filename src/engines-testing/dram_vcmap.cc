// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "dram_vcmap.h"

namespace pmem
{
namespace kv
{

template <>
std::string dram_vcmap::name()
{
	return "dram_vcmap";
}

} // namespace kv
} // namespace pmem
