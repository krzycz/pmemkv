// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#ifndef LIBPMEMKV_H
#define LIBPMEMKV_H

#include <libpmemobj/pool_base.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PMEMKV_STATUS_OK 0
#define PMEMKV_STATUS_UNKNOWN_ERROR 1
#define PMEMKV_STATUS_NOT_FOUND 2
#define PMEMKV_STATUS_NOT_SUPPORTED 3
#define PMEMKV_STATUS_INVALID_ARGUMENT 4
#define PMEMKV_STATUS_CONFIG_PARSING_ERROR 5
#define PMEMKV_STATUS_CONFIG_TYPE_ERROR 6
#define PMEMKV_STATUS_STOPPED_BY_CB 7
#define PMEMKV_STATUS_OUT_OF_MEMORY 8
#define PMEMKV_STATUS_WRONG_ENGINE_NAME 9
#define PMEMKV_STATUS_TRANSACTION_SCOPE_ERROR 10
#define PMEMKV_STATUS_DEFRAG_ERROR 11
#define PMEMKV_STATUS_COMPARATOR_MISMATCH 12

typedef struct pmemkv_db pmemkv_db;
typedef struct pmemkv_config pmemkv_config;
typedef struct pmemkv_comparator pmemkv_comparator;
typedef struct pmemkv_tx pmemkv_tx;

typedef struct pmemkv_iterator pmemkv_iterator;
typedef struct {
	pmemkv_iterator *iter;
} pmemkv_write_iterator;

typedef int pmemkv_get_kv_callback(const char *key, size_t keybytes, const char *value,
				   size_t valuebytes, void *arg);
typedef void pmemkv_get_v_callback(const char *value, size_t valuebytes, void *arg);

typedef int pmemkv_compare_function(const char *key1, size_t keybytes1, const char *key2,
				    size_t keybytes2, void *arg);

pmemkv_comparator *pmemkv_comparator_new(pmemkv_compare_function *fn, const char *name,
					 void *arg);
void pmemkv_comparator_delete(pmemkv_comparator *comparator);

pmemkv_config *pmemkv_config_new(void);
void pmemkv_config_delete(pmemkv_config *config);
int pmemkv_config_put_data(pmemkv_config *config, const char *key, const void *value,
			   size_t value_size);
int pmemkv_config_put_object(pmemkv_config *config, const char *key, void *value,
			     void (*deleter)(void *));
int pmemkv_config_put_object_cb(pmemkv_config *config, const char *key, void *value,
				void *(*getter)(void *), void (*deleter)(void *));
int pmemkv_config_put_uint64(pmemkv_config *config, const char *key, uint64_t value);
int pmemkv_config_put_int64(pmemkv_config *config, const char *key, int64_t value);
int pmemkv_config_put_string(pmemkv_config *config, const char *key, const char *value);
int pmemkv_config_get_data(pmemkv_config *config, const char *key, const void **value,
			   size_t *value_size);
int pmemkv_config_get_object(pmemkv_config *config, const char *key, void **value);
int pmemkv_config_get_uint64(pmemkv_config *config, const char *key, uint64_t *value);
int pmemkv_config_get_int64(pmemkv_config *config, const char *key, int64_t *value);
int pmemkv_config_get_string(pmemkv_config *config, const char *key, const char **value);

int pmemkv_config_put_size(pmemkv_config *config, uint64_t value);
int pmemkv_config_put_path(pmemkv_config *config, const char *value);
int pmemkv_config_put_force_create(pmemkv_config *config, bool value);
int pmemkv_config_put_comparator(pmemkv_config *config, pmemkv_comparator *comparator);
int pmemkv_config_put_oid(pmemkv_config *config, PMEMoid *oid);
int pmemkv_config_reserve(pmemkv_config *config, uint64_t value);

int pmemkv_open(const char *engine, pmemkv_config *config, pmemkv_db **db);
void pmemkv_close(pmemkv_db *kv);

int pmemkv_count_all(pmemkv_db *db, size_t *cnt);
int pmemkv_count_above(pmemkv_db *db, const char *k, size_t kb, size_t *cnt);
int pmemkv_count_equal_above(pmemkv_db *db, const char *k, size_t kb, size_t *cnt);
int pmemkv_count_equal_below(pmemkv_db *db, const char *k, size_t kb, size_t *cnt);
int pmemkv_count_below(pmemkv_db *db, const char *k, size_t kb, size_t *cnt);
int pmemkv_count_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
			 size_t kb2, size_t *cnt);

int pmemkv_get_all(pmemkv_db *db, pmemkv_get_kv_callback *c, void *arg);
int pmemkv_get_above(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c,
		     void *arg);
int pmemkv_get_equal_above(pmemkv_db *db, const char *k, size_t kb,
			   pmemkv_get_kv_callback *c, void *arg);
int pmemkv_get_equal_below(pmemkv_db *db, const char *k, size_t kb,
			   pmemkv_get_kv_callback *c, void *arg);
int pmemkv_get_below(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c,
		     void *arg);
int pmemkv_get_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
		       size_t kb2, pmemkv_get_kv_callback *c, void *arg);

int pmemkv_exists(pmemkv_db *db, const char *k, size_t kb);

int pmemkv_get(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_v_callback *c,
	       void *arg);
int pmemkv_get_copy(pmemkv_db *db, const char *k, size_t kb, char *buffer,
		    size_t buffer_size, size_t *value_size);
int pmemkv_put(pmemkv_db *db, const char *k, size_t kb, const char *v, size_t vb);

int pmemkv_remove(pmemkv_db *db, const char *k, size_t kb);

int pmemkv_defrag(pmemkv_db *db, double start_percent, double amount_percent);

const char *pmemkv_errormsg(void);

/* This API is EXPERIMENTAL and might change. */
int pmemkv_tx_begin(pmemkv_db *db, pmemkv_tx **tx);
int pmemkv_tx_put(pmemkv_tx *tx, const char *k, size_t kb, const char *v, size_t vb);
int pmemkv_tx_remove(pmemkv_tx *tx, const char *k, size_t kb);
int pmemkv_tx_commit(pmemkv_tx *tx);
void pmemkv_tx_abort(pmemkv_tx *tx);
void pmemkv_tx_end(pmemkv_tx *tx);

/* This API is EXPERIMENTAL and might change. */
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

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* LIBPMEMKV_H */
