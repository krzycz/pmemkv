---
layout: manual
Content-Style: 'text/css'
title: PMEMKV_JSON_CONFIG
collection: libpmemkv
header: PMEMKV_JSON_CONFIG
secondary_title: pmemkv_json_config
...

[comment]: <> (SPDX-License-Identifier: BSD-3-Clause)
[comment]: <> (Copyright 2019-2021, Intel Corporation)

[comment]: <> (libpmemkv_json_config.3 -- man page for libpmemkv_json_config configuration API)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[ERRORS](#errors)<br />
[EXAMPLE](#example)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**pmemkv_json_config** - helper configuration API for libpmemkv

# SYNOPSIS #

```c
#include <libpmemkv_json_config.h>

int pmemkv_config_from_json(pmemkv_config *config, const char *jsonconfig);
const char *pmemkv_config_from_json_errormsg(void);
```

For general description of pmemkv and available engines see **libpmemkv**(7).
For description of pmemkv core API see **libpmemkv**(3).
For description of configuration API for libpmemkv see **libpmemkv_config**(3).

# DESCRIPTION #

pmemkv_json_config is a helper library that provides two functions:

`int pmemkv_config_from_json(pmemkv_config *config, const char *jsonconfig);`

:	Parses JSON string and puts all items found in JSON into `config`. Allowed types
	in JSON strings and their corresponding types in pmemkv_config are:
	+ **number** -- int64 or uint64
	+ **string** -- const char *
	+ **object** -- (another JSON string) -> pointer to pmemkv_config (can be obtained using pmemkv_config_get_object)
	+ **True**, **False** -- int64

`const char *pmemkv_config_from_json_errormsg(void);`

:	Returns a human readable string describing the last error.

The `pmemkv_config_from_json` function depends on RapidJSON library
what is the direct cause of the creation of this small library.

The building of this library is enabled by default.
It can be disabled by setting the **BUILD_JSON_CONFIG** CMake variable to OFF:

```sh
cmake .. -DBUILD_JSON_CONFIG=OFF
```

## ERRORS ##

The `pmemkv_config_from_json()` function returns status. Possible return values are:

+ **PMEMKV_STATUS_OK** -- no error
+ **PMEMKV_STATUS_UNKNOWN_ERROR** -- unknown error
+ **PMEMKV_STATUS_CONFIG_PARSING_ERROR** -- parsing config data failed

# EXAMPLE #

An example using this API can be found in **libpmemkv_config**(3).

# SEE ALSO #

**libpmemkv**(7), **libpmemkv**(3), **libpmemkv_config**(3) and **<https://pmem.io>**
