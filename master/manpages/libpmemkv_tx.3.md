---
layout: manual
Content-Style: 'text/css'
title: PMEMKV_TX
collection: libpmemkv
header: PMEMKV_TX
secondary_title: pmemkv
...

[comment]: <> (SPDX-License-Identifier: BSD-3-Clause)
[comment]: <> (Copyright 2020-2021, Intel Corporation)

[comment]: <> (libpmemkv_tx.3 -- man page for libpmemkv transactions API)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[ERRORS](#errors)<br />
[EXAMPLE](#example)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**pmemkv_tx** - Transactions API for libpmemkv

This API is **EXPERIMENTAL** and might change.

# SYNOPSIS #

```c
#include <libpmemkv.h>

int pmemkv_tx_begin(pmemkv_db *db, pmemkv_tx **tx);
int pmemkv_tx_put(pmemkv_tx *tx, const char *k, size_t kb, const char *v, size_t vb);
int pmemkv_tx_remove(pmemkv_tx *tx, const char *k, size_t kb);
int pmemkv_tx_commit(pmemkv_tx *tx);
void pmemkv_tx_abort(pmemkv_tx *tx);
void pmemkv_tx_end(pmemkv_tx *tx);
```

# DESCRIPTION #

The transaction allows grouping `put` and `remove` operations into a single atomic action
(with respect to persistence and concurrency). Concurrent engines provide transactions
with ACID (atomicity, consistency, isolation, durability) properties. Transactions for
single threaded engines provide atomicity, consistency and durability. Actions in a transaction
are executed in the order in which they were called.

`int pmemkv_tx_begin(pmemkv_db *db, pmemkv_tx **tx);`

:	Starts a pmemkv transaction and stores a pointer to a *pmemkv_tx* instance in `*tx`.

`int pmemkv_tx_put(pmemkv_tx *tx, const char *k, size_t kb, const char *v, size_t vb);`

:   Inserts a key-value pair into pmemkv database. `kb` is the length of the key `k` and `vb` is the length of value `v`.
	When this function returns, caller is free to reuse both buffers. The inserted element is visible only after calling pmemkv_tx_commit.


`int pmemkv_tx_remove(pmemkv_tx *tx, const char *k, size_t kb);`

:   Removes record with the key `k` of length `kb`. The removed elements are still visible until calling pmemkv_tx_commit.
	This function will succeed even if there is no element in the database.


`int pmemkv_tx_commit(pmemkv_tx *tx);`

:   Commits the transaction. All operations of this transaction are applied as a single power fail-safe atomic action.

`void pmemkv_tx_abort(pmemkv_tx *tx);`

:   Discards all uncommitted operations.

`void pmemkv_tx_end(pmemkv_tx *tx);`

:	Deletes the pmemkv transaction object and discards all uncommitted operations.

## ERRORS ##

Each function, except for *pmemkv_tx_abort()* and *pmemkv_tx_end()* returns status. Possible return values are listed in **libpmemkv**(3).

# EXAMPLE #

The following example is taken from `examples/pmemkv_transaction_c` directory.

Usage of pmemkv transaction in C:

```c
#include <assert.h>
#include <libpmemkv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			puts(pmemkv_errormsg());                                         \
		assert(expr);                                                            \
	} while (0)

#define LOG(msg) puts(msg)

/*
 * This example expects a path to already created database pool.
 *
 * To create a pool use one of the following commands.
 *
 * For regular pools use:
 * pmempool create -l -s 1G "pmemkv_radix" obj path_to_a_pool
 *
 * For poolsets use:
 * pmempool create -l "pmemkv_radix" obj ../examples/example.poolset
 */
int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s pool\n", argv[0]);
		exit(1);
	}

	/* See libpmemkv_config(3) for more detailed example of config creation */
	LOG("Creating config");
	pmemkv_config *cfg = pmemkv_config_new();
	ASSERT(cfg != NULL);

	int s = pmemkv_config_put_path(cfg, argv[1]);
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Opening pmemkv database with 'radix' engine");
	pmemkv_db *db = NULL;
	s = pmemkv_open("radix", cfg, &db);
	ASSERT(s == PMEMKV_STATUS_OK);
	ASSERT(db != NULL);

	const char *key1 = "key1";
	const char *value1 = "value1";
	const char *key2 = "key2";
	const char *value2 = "value2";
	const char *key3 = "key3";
	const char *value3 = "value3";

	LOG("Putting new key");
	s = pmemkv_put(db, key1, strlen(key1), value1, strlen(value1));
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Starting a tx");
	pmemkv_tx *tx;
	s = pmemkv_tx_begin(db, &tx);
	ASSERT(s == PMEMKV_STATUS_OK);

	s = pmemkv_tx_remove(tx, key1, strlen(key1));
	s = pmemkv_tx_put(tx, key2, strlen(key2), value2, strlen(value2));
	s = pmemkv_tx_put(tx, key3, strlen(key3), value3, strlen(value3));

	/* Until transaction is committed, changes are not visible */
	s = pmemkv_exists(db, key1, strlen(key1));
	ASSERT(s == PMEMKV_STATUS_OK);
	s = pmemkv_exists(db, key2, strlen(key2));
	ASSERT(s == PMEMKV_STATUS_NOT_FOUND);
	s = pmemkv_exists(db, key3, strlen(key3));
	ASSERT(s == PMEMKV_STATUS_NOT_FOUND);

	s = pmemkv_tx_commit(tx);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* Changes are now visible and pmemkv_tx object is destroyed */
	s = pmemkv_exists(db, key1, strlen(key1));
	ASSERT(s == PMEMKV_STATUS_NOT_FOUND);
	s = pmemkv_exists(db, key2, strlen(key2));
	ASSERT(s == PMEMKV_STATUS_OK);
	s = pmemkv_exists(db, key3, strlen(key3));
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Ending transaction");
	pmemkv_tx_end(tx);

	LOG("Closing database");
	pmemkv_close(db);

	return 0;
}

```

# SEE ALSO #

**libpmemkv**(7), **libpmemkv**(3) and **<https://pmem.io>**
