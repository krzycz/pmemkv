---
layout: manual
Content-Style: 'text/css'
title: _MP(PMEMKV, 7)
collection: libpmemkv
header: PMEMKV
secondary_title: pmemkv
...

[comment]: <> (SPDX-License-Identifier: BSD-3-Clause)
[comment]: <> (Copyright 2019-2020, Intel Corporation)

[comment]: <> (libpmemkv.7 -- man page for libpmemkv)

[NAME](#name)<br />
[DESCRIPTION](#description)<br />
[ENGINES](#engines)<br />
[BINDINGS](#bindings)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**pmemkv** - Key/Value Datastore for Persistent Memory

# DESCRIPTION #

**pmemkv** is a key-value datastore framework optimized for persistent memory. It provides native C API and C++ headers. Support for other languages is described in the **BINDINGS** section below.

It has multiple storage engines, each optimized for a different use case. They differ in implementation and capabilities:

+ persistence - this is a trade-off between data preservation and performance; persistent engines retain their content and are power fail/crash safe, but are slower; volatile engines are faster, but keep their content only until the database is closed (or application crashes; power fail occurs)

+ concurrency - engines provide a varying degree of write scalability in multi-threaded workloads. Concurrent engines support non-blocking retrievals and, on average, highly scalable updates. For details see the description of individual engines.

+ keys' ordering - "sorted" engines support querying above/below the given key. Most sorted engines also support passing a custom comparator object (see **libpmemkv_config**(3)). By default, pmemkv stores elements in binary order (comparison is done using a function equivalent to std::string::compare).

Persistent engines usually use libpmemobj++ and PMDK to access NVDIMMs. They can work with files on DAX filesystem (fsdax) or DAX device.

# ENGINES #

| Engine Name  | Description | Persistent? | Concurrent? | Sorted? |
| ------------ | ----------- | ----------- | ----------- | ------- |
| **cmap** | **Concurrent hash map** | **Yes** | **Yes** | **No** |
| vcmap | Volatile concurrent hash map | No | Yes | No |
| vsmap | Volatile sorted hash map | No | No | Yes |
| blackhole | Accepts everything, returns nothing | No | Yes | No |

The most mature and recommended engine to use for persistent use-cases is **cmap**. It provides good performance results and stability.

Each engine can be manually turned on and off at build time, using CMake options. All engines listed here are enabled and ready to use.

To configure an engine, pmemkv_config is used (**libpmemkv_config**(3)). Below is a list of engines along with config parameters they expect. Each parameter has corresponding function (pmemkv_config_put_path, pmemkv_config_put_comparator, etc.), which guarantees type safety. For example to insert `path` parameter to the config, user should call pmemkv_config_put_path().
For some use cases, like creating config from parsed input, it may be more convinient to insert parameters by its type instead of name. Each paramter has a certain type and may be inserted to a config using appropriate function (pmemkv_config_put_string, pmemkv_config_put_int64, etc.). For example, to insert a parameter of type `string`, `pmemkv_config_put_string` function may be used.
Those two ways of inserting parameters into config may be used interchangeably.

For description of pmemkv core API see **libpmemkv**(3).

## cmap

A persistent concurrent engine, backed by a hashmap that allows calling get, put, and remove concurrently from multiple threads and ensures good scalability. Rest of the methods (e.g. range query methods) are not thread-safe and should not be called from more than one thread.
Data stored using this engine is persistent and guaranteed to be consistent in case of any kind of interruption (crash / power loss / etc).

Internally this engine uses persistent concurrent hashmap and persistent string from libpmemobj-cpp library (for details see <https://github.com/pmem/libpmemobj-cpp>). Persistent string is used as a type of a key and a value. Engine's functions should not be called within libpmemobj transactions (improper call by user will result thrown exception).

This engine requires the following config parameters (see **libpmemkv_config**(3) for details how to set them):

* **path** -- Path to a database file or to a poolset file (see **poolset**(5) for details). Note that when using poolset file, size should be 0
	+ type: string
* **force_create** -- If 0, pmemkv opens file specified by 'path', otherwise it creates it.
	+ type: uint64_t
	+ default value: 0
* **size** --  Only needed when force_create is not 0, specifies size of the database [in bytes].
	+ type: uint64_t
	+ min value: 8388608 (8MB)
* **oid** -- Pointer to oid (for details see **libpmemobj**(7)) which points to engine data. If oid is null, engine will allocate new data, otherwise it will use existing one.
	+ type: object
* **pmemobj_stats_enabled** -- Enables or disables runtime collection of statistics. There are two types of statistics: persistent and transient ones. Persistent statistics survive pool restarts, whereas transient ones don’t. Statistics are not recalculated after enabling; any operations that occur between disabling and re-enabling will not be reflected in subsequent values.
If pmemobj_stats_enabled is set to 0, only transcient statistics are enabled. If set to 1, both transcient and persistent statistics are enabled. If set to 2, only persistent statistics are enabled. If set to 3, all statistics are disabled. Only transient statistics are enabled by default. Enabling persistent statistics may have non-trivial performance impact.
Currently available statistics are: stats.heap.curr_allocated, stats.heap.run_allocated, stats.heap.run_active (for details see <https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3>). These statistics are being gathered per persistent memory pool, not by en engine.
	+ type: uint64_t
	+ default value: 0

The following table shows three possible combinations of parameters (where '-' means 'cannot be set'):

| **#** | **path** | **force_create** | **size** | **oid** |
| ----- | -------- | ---------------- | -------- | ------- |
| **1** | set | 0 | - | - |
| **2** | set | 1 | set | - |
| **3** | - | - | - | set |

A database file or a poolset file can also be created using **pmempool** utility (see **pmempool-create**(1)).
When using **pmempool create**, "pmemkv" should be passed as layout for cmap engine and "pmemkv_\<engine-name\>" for other engines (e.g. "pmemkv_stree" for stree engine). Only PMEMOBJ pools are supported.

## vcmap

A volatile concurrent engine, backed by memkind. Data written using this engine is lost after database is closed.

This engine is built on top of tbb::concurrent\_hash\_map data structure and uses PMEM C++ allocator to allocate memory. std::basic\_string is used as a type of a key and a value.
Memkind and TBB packages are required.

This engine requires the following config parameters (see **libpmemkv_config**(3) for details how to set them):

* **path** -- Path to an existing directory
	+ type: string
* **size** --  Specifies size of the database [in bytes]
	+ type: uint64_t
	+ min value: 8388608 (8MB)

## vsmap

A volatile single-threaded sorted engine, backed by memkind. Data written using this engine is lost after database is closed.

This engine is built on top of std::map and uses PMEM C++ allocator to allocate memory. std::basic\_string is used as a type of a key and a value.
Memkind package is required.

This engine requires the following config parameters (see **libpmemkv_config**(3) for details how to set them):

* **path** -- Path to an existing directory
	+ type: string
* **size** --  Specifies size of the database [in bytes]
	+ type: uint64_t
	+ min value: 8388608 (8MB)
* **comparator** -- (optional) Specified comparator used by the engine
	+ type: object

## blackhole

A volatile engine that accepts an unlimited amount of data, but never returns anything.
Internally, `blackhole` does not use a persistent pool or any durable structure. The intended use of this engine is to profile and tune high-level bindings, and similar cases when persistence
should be intentionally skipped.
No additional packages are required.
No supported configuration parameters.

### Experimental engines

There are also more engines in various states of development, for details see <https://github.com/pmem/pmemkv/blob/master/doc/ENGINES-experimental.md>.
Some of them (radix, tree3, stree and csmap) requires the config parameters like cmap and similarly to cmap should not be used within libpmemobj transaction(s).

# BINDINGS #

Bindings for other languages are available on GitHub. Currently they support only subset of native API.

Existing bindings:

+ Java - for details see <https://github.com/pmem/pmemkv-java>

+ Node.js - for details see <https://github.com/pmem/pmemkv-nodejs>

+ Python - for details see <https://github.com/pmem/pmemkv-python>

+ Ruby - for details see <https://github.com/pmem/pmemkv-ruby>

# SEE ALSO #

**libpmemkv**(3), **libpmemkv_config(3)**, **pmempool**(1), **libpmemobj**(7) and **<https://pmem.io>**
