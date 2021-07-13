---
layout: manual
Content-Style: 'text/css'
title: PMEMKV_ITERATOR
collection: libpmemkv
header: PMEMKV_ITERATOR
secondary_title: pmemkv
...

[comment]: <> (SPDX-License-Identifier: BSD-3-Clause)
[comment]: <> (Copyright 2020-2021, Intel Corporation)

[comment]: <> (libpmemkv_iterator.3 -- man page for libpmemkv iterators API)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[ERRORS](#errors)<br />
[EXAMPLE](#example)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**pmemkv_iterator** - Iterator API for libpmemkv

This API is **EXPERIMENTAL** and might change.

# SYNOPSIS #

```c
#include <libpmemkv.h>

int pmemkv_iterator_new(pmemkv_db *db, pmemkv_iterator **it);
int pmemkv_write_iterator_new(pmemkv_db *db, pmemkv_write_iterator **it);

void pmemkv_iterator_delete(pmemkv_iterator *it);
void pmemkv_write_iterator_delete(pmemkv_write_iterator *it);

int pmemkv_iterator_seek(pmemkv_iterator *it, const char *k, size_t kb);
int pmemkv_iterator_seek_lower(pmemkv_iterator *it, const char *k, size_t kb);
int pmemkv_iterator_seek_lower_eq(pmemkv_iterator *it, const char *k, size_t kb);
int pmemkv_iterator_seek_higher(pmemkv_iterator *it, const char *k, size_t kb);
int pmemkv_iterator_seek_higher_eq(pmemkv_iterator *it, const char *k, size_t kb);

int pmemkv_iterator_seek_to_first(pmemkv_iterator *it);
int pmemkv_iterator_seek_to_last(pmemkv_iterator *it);

int pmemkv_iterator_is_next(pmemkv_iterator *it);
int pmemkv_iterator_next(pmemkv_iterator *it);
int pmemkv_iterator_prev(pmemkv_iterator *it);

int pmemkv_iterator_key(pmemkv_iterator *it, const char **k, size_t *kb);

int pmemkv_iterator_read_range(pmemkv_iterator *it, size_t pos, size_t n,
					const char **data, size_t *rb);
int pmemkv_write_iterator_write_range(pmemkv_write_iterator *it, size_t pos, size_t n,
					char **data, size_t *wb);

int pmemkv_write_iterator_commit(pmemkv_write_iterator *it);
void pmemkv_write_iterator_abort(pmemkv_write_iterator *it);
```

For general description of pmemkv and available engines see **libpmemkv**(7).
For description of pmemkv core API see **libpmemkv**(3).

# DESCRIPTION #

Iterators provide methods to iterate over records in db.

Both iterator types (pmemkv_iterator (read) and pmemkv_write_iterator) allow reading record's
key and value. To use pmemkv_write_iterator as a pmemkv_iterator you need to get its member "iter" (write_it->iter).

Example of calling pmemkv_iterator_seek_to_first() with both iterator types.
```c
/* read_it is already created, by pmemkv_iterator */
int status = pmemkv_iterator_seek_to_first(read_it);

/* write_it is already created, by pmemkv_write_iterator */
int status = pmemkv_iterator_seek_to_first(write_it->iter);
```

A pmemkv_write_iterator additionally can modify record's value transactionally.

Some of the functions are not guaranteed to be implemented by all engines.
If an engine does not support a certain function, it will return PMEMKV\_STATUS\_NOT\_SUPPORTED.

Holding simultaneously in the same thread more than one iterator is undefined behavior.

`int pmemkv_iterator_new(pmemkv_db *db, pmemkv_iterator **it);`

:	Creates a new pmemkv_iterator instance and stores a pointer to it in `*it`.

`int pmemkv_write_iterator_new(pmemkv_db *db, pmemkv_write_iterator **it);`

:	Creates a new pmemkv_write_iterator instance and stores a pointer to it in `*it`.

`void pmemkv_iterator_delete(pmemkv_iterator *it);`

:	Deletes pmemkv_iterator.

`void pmemkv_write_iterator_delete(pmemkv_write_iterator *it);`

:	Deletes pmemkv_write_iterator

`int pmemkv_iterator_seek(pmemkv_iterator *it, const char *k, size_t kb);`

:	Changes iterator position to the record with given key `k` of length `kb`.
	If the record is present and no errors occurred, returns PMEMKV_STATUS_OK.
	If the record does not exist, PMEMKV_STATUS_NOT_FOUND is returned and the iterator
	position is undefined.
	It internally aborts all changes made to an element previously pointed by the iterator.

`int pmemkv_iterator_seek_lower(pmemkv_iterator *it, const char *k, size_t kb);`

:	Changes iterator position to the record with key lower than given key `k` of length `kb`.
	If the record is present and no errors occurred, returns PMEMKV_STATUS_OK.
	If the record does not exist, PMEMKV_STATUS_NOT_FOUND is returned and the iterator
	position is undefined.
	It internally aborts all changes made to an element previously pointed by the iterator.

`int pmemkv_iterator_seek_lower_eq(pmemkv_iterator *it, const char *k, size_t kb);`

:	Changes iterator position to the record with key equal or lower than given key `k` of length `kb`.
	If the record is present and no errors occurred, returns PMEMKV_STATUS_OK.
	If the record does not exist, PMEMKV_STATUS_NOT_FOUND is returned and the iterator
	position is undefined.
	It internally aborts all changes made to an element previously pointed by the iterator.

`int pmemkv_iterator_seek_higher(pmemkv_iterator *it, const char *k, size_t kb);`

:	Changes iterator position to the record with key higher than given key `k` of length `kb`.
	If the record is present and no errors occurred, returns PMEMKV_STATUS_OK.
	If the record does not exist, PMEMKV_STATUS_NOT_FOUND is returned and the iterator
	position is undefined.
	It internally aborts all changes made to an element previously pointed by the iterator.

`int pmemkv_iterator_seek_higher_eq(pmemkv_iterator *it, const char *k, size_t kb);`

:	Changes iterator position to the record with key equal or higher than given key `k` of length `kb`.
	If the record is present and no errors occurred, returns PMEMKV_STATUS_OK.
	If the record does not exist, PMEMKV_STATUS_NOT_FOUND is returned and the iterator
	position is undefined.
	It internally aborts all changes made to an element previously pointed by the iterator.

`int pmemkv_iterator_seek_to_first(pmemkv_iterator *it);`

:	Changes iterator position to the first record. If db isn't empty, and no errors occurred, returns
	PMEMKV_STATUS_OK. If db is empty, PMEMKV_STATUS_NOT_FOUND is returned and the iterator position is undefined.
	It internally aborts all changes made to an element previously pointed by the iterator.

`int pmemkv_iterator_seek_to_last(pmemkv_iterator *it);`

:	Changes iterator position to the last record. If db isn't empty, and no errors occurred, returns
	PMEMKV_STATUS_OK. If db is empty, PMEMKV_STATUS_NOT_FOUND is returned and the iterator position is undefined.
	It internally aborts all changes made to an element previously pointed by the iterator.

`int pmemkv_iterator_is_next(pmemkv_iterator *it);`

:	Checks if there is a next record available. If true is returned, it is guaranteed that
	pmemkv_iterator_next(it) will return PMEMKV_STATUS_OK, otherwise iterator is already on the last
	element and pmemkv_iterator_next(it) will return PMEMKV_STATUS_NOT_FOUND.

`int pmemkv_iterator_next(pmemkv_iterator *it);`

:	Changes iterator position to the next record.
	If the next record exists, returns PMEMKV_STATUS_OK, otherwise
	PMEMKV_STATUS_NOT_FOUND is returned and the iterator position is undefined.
	It internally aborts all changes made to an element previously pointed by the iterator.

`int pmemkv_iterator_prev(pmemkv_iterator *it);`

:	Changes iterator position to the previous record.
	If the previous record exists, returns PMEMKV_STATUS_OK, otherwise
	PMEMKV_STATUS_NOT_FOUND is returned and the iterator position is undefined.
	It internally aborts all changes made to an element previously pointed by the iterator.

`int pmemkv_iterator_key(pmemkv_iterator *it, const char **k, size_t *kb);`

:	Assigns record's key's address to `k` and key's length to `kb`. If the iterator is on an undefined position,
	calling this method is undefined behaviour.

`int pmemkv_iterator_read_range(pmemkv_iterator *it, size_t pos, size_t n, const char **data, size_t *rb);`

:	Allows getting record's value's range which can be only read.
	You can request for either full value or only value's subrange (`n` elements starting from `pos`).
	Assigns pointer to the beginning of the requested range to `data`, and number of elements in range to `rb`.
	If `n` is bigger than length of a value it's automatically shrunk.
	If the iterator is on an undefined position, calling this method is undefined behaviour.

`int pmemkv_write_iterator_write_range(pmemkv_write_iterator *it, size_t pos, size_t n, char **data, size_t *wb);`

:	Allows getting record's value's range which can be modified.
	You can request for either full value or only value's subrange (`n` elements starting from `pos`).
	Assigns pointer to the beginning of the requested range to `data`, and number of elements in range to `wb`.
	If `n` is bigger than length of a value it's automatically shrunk.
	Changes made on a requested range are not persistent until *pmemkv_write_iterator_commit()* is called.
	If the iterator is on an undefined position, calling this method is undefined behaviour.

`int pmemkv_write_iterator_commit(pmemkv_write_iterator *it);`

:	Commits modifications made on the current record.
	Calling this method is the only way to save modifications made by the iterator on the current
	record. You need to call this method before changing the iterator position, otherwise
	modifications will be automatically aborted.

`void pmemkv_write_iterator_abort(pmemkv_write_iterator *it);`

:	Aborts uncommitted modifications made on the current record.

## ERRORS ##

Each function, except for *pmemkv_iterator_delete()*, *pmemkv_write_iterator_delete()* and *pmemkv_write_iterator_abort()*,
returns one of the pmemkv status codes. To check possible options see **libpmemkv**(3).

# EXAMPLE #

The following example is taken from `examples/pmemkv_iterator_c` directory.

Usage of basic iterator functions to iterate over all records and modify one of them.

```c
#include "libpmemkv.h"

#include <assert.h>
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

static const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		exit(1);
	}

	const size_t n_elements = 10;
	char buffer[64];

	/* See libpmemkv_config(3) for more detailed example of config creation */
	LOG("Creating config");
	pmemkv_config *cfg = pmemkv_config_new();
	ASSERT(cfg != NULL);

	int s = pmemkv_config_put_path(cfg, argv[1]);
	ASSERT(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put_size(cfg, SIZE);
	ASSERT(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put_create_if_missing(cfg, true);
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Opening pmemkv database with 'radix' engine");
	pmemkv_db *db = NULL;
	s = pmemkv_open("radix", cfg, &db);
	ASSERT(s == PMEMKV_STATUS_OK);
	ASSERT(db != NULL);

	LOG("Putting new keys");
	for (size_t i = 0; i < n_elements; ++i) {
		char key[10];
		const char *value = "value";
		sprintf(key, "key%zu", i);
		s = pmemkv_put(db, key, strlen(key), value, strlen(value));
		ASSERT(s == PMEMKV_STATUS_OK);
	}

	/* get a new read iterator */
	pmemkv_iterator *it;
	s = pmemkv_iterator_new(db, &it);
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Iterate from first to last element");
	s = pmemkv_iterator_seek_to_first(it);
	ASSERT(s == PMEMKV_STATUS_OK);

	size_t element_number = 0;
	do {
		const char *str;
		size_t cnt;
		/* read a key */
		s = pmemkv_iterator_key(it, &str, &cnt);
		ASSERT(s == PMEMKV_STATUS_OK);
		sprintf(buffer, "Key %zu = %s", element_number++, str);
		LOG(buffer);
	} while (pmemkv_iterator_next(it) != PMEMKV_STATUS_NOT_FOUND);

	/* iterator must be deleted manually */
	pmemkv_iterator_delete(it);

	/* get a new write_iterator */
	pmemkv_write_iterator *w_it;
	s = pmemkv_write_iterator_new(db, &w_it);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* if you want to get a pmemkv_iterator (read iterator) from a
	 * pmemkv_write_iterator, you should do: write_it->iter */
	s = pmemkv_iterator_seek_to_last(w_it->iter);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* get a write range, to modify last element's value */
	char *data;
	size_t cnt;
	s = pmemkv_write_iterator_write_range(w_it, 0, 5, &data, &cnt);
	ASSERT(s == PMEMKV_STATUS_OK);

	for (size_t i = 0; i < cnt; ++i)
		data[i] = 'x';

	/* commit changes */
	s = pmemkv_write_iterator_commit(w_it);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* get a read range, to read modified value */
	const char *str;
	s = pmemkv_iterator_read_range(w_it->iter, 0, 5, &str, &cnt);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* verify a modified value */
	ASSERT(strcmp(str, "xxxxx") == 0);
	sprintf(buffer, "Modified value = %s", str);
	LOG(buffer);

	/* iterator must be deleted manually */
	pmemkv_write_iterator_delete(w_it);

	LOG("Closing database");
	pmemkv_close(db);

	return 0;
}

```

# SEE ALSO #

**libpmemkv**(7), **libpmemkv**(3) and **<https://pmem.io>**
