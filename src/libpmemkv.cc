// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include <memory>

#include <sys/stat.h>

#include "comparator/comparator.h"
#include "config.h"
#include "engine.h"
#include "exceptions.h"
#include "libpmemkv.h"
#include "libpmemkv.hpp"
#include "libpmemobj++/pexceptions.hpp"
#include "out.h"

#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

static inline pmemkv_config *config_from_internal(pmem::kv::internal::config *config)
{
	return reinterpret_cast<pmemkv_config *>(config);
}

static inline pmem::kv::internal::config *config_to_internal(pmemkv_config *config)
{
	return reinterpret_cast<pmem::kv::internal::config *>(config);
}

static inline pmemkv_comparator *
comparator_from_internal(pmem::kv::internal::comparator *comparator)
{
	return reinterpret_cast<pmemkv_comparator *>(comparator);
}

static inline pmem::kv::internal::comparator *
comparator_to_internal(pmemkv_comparator *comparator)
{
	return reinterpret_cast<pmem::kv::internal::comparator *>(comparator);
}

static inline pmem::kv::engine_base *db_to_internal(pmemkv_db *db)
{
	return reinterpret_cast<pmem::kv::engine_base *>(db);
}

static inline pmemkv_db *db_from_internal(pmem::kv::engine_base *db)
{
	return reinterpret_cast<pmemkv_db *>(db);
}

template <typename Function>
static inline int catch_and_return_status(const char *func_name, Function &&f)
{
	try {
		return static_cast<int>(f());
	} catch (pmem::kv::internal::error &e) {
		out_err_stream(func_name) << e.what();
		return e.status_code;
	} catch (std::bad_alloc &e) {
		out_err_stream(func_name) << e.what();
		return PMEMKV_STATUS_OUT_OF_MEMORY;
	} catch (std::runtime_error &e) {
		out_err_stream(func_name) << e.what();
		return PMEMKV_STATUS_UNKNOWN_ERROR;
	} catch (pmem::transaction_scope_error &e) {
		out_err_stream(func_name) << e.what();
		return PMEMKV_STATUS_TRANSACTION_SCOPE_ERROR;
	} catch (std::exception &e) {
		out_err_stream(func_name) << e.what();
		return PMEMKV_STATUS_UNKNOWN_ERROR;
	} catch (...) {
		out_err_stream(func_name) << "Unspecified error";
		return PMEMKV_STATUS_UNKNOWN_ERROR;
	}
}

extern "C" {

pmemkv_config *pmemkv_config_new(void)
{
	try {
		return config_from_internal(new pmem::kv::internal::config);
	} catch (const std::exception &exc) {
		ERR() << exc.what();
		return nullptr;
	} catch (...) {
		ERR() << "Unspecified failure";
		return nullptr;
	}
}

void pmemkv_config_delete(pmemkv_config *config)
{
	try {
		delete config_to_internal(config);
	} catch (const std::exception &exc) {
		ERR() << exc.what();
	} catch (...) {
		ERR() << "Unspecified failure";
	}
}

int pmemkv_config_put_data(pmemkv_config *config, const char *key, const void *value,
			   size_t value_size)
{
	if (!config)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_data(key, value, value_size);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_object(pmemkv_config *config, const char *key, void *value,
			     void (*deleter)(void *))
{
	if (!config)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_object(key, value, deleter);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_object_cb(pmemkv_config *config, const char *key, void *value,
				void *(*getter)(void *), void (*deleter)(void *))
{
	if (!config || !getter)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_object(key, value, deleter, getter);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_int64(pmemkv_config *config, const char *key, int64_t value)
{
	if (!config)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_int64(key, value);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_uint64(pmemkv_config *config, const char *key, uint64_t value)
{
	if (!config)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_uint64(key, value);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_string(pmemkv_config *config, const char *key, const char *value)
{
	if (!config)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_string(key, value);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_size(pmemkv_config *config, uint64_t value)
{
	return pmemkv_config_put_uint64(config, "size", value);
}

int pmemkv_config_put_path(pmemkv_config *config, const char *value)
{
	return pmemkv_config_put_string(config, "path", value);
}

int pmemkv_config_put_force_create(pmemkv_config *config, bool value)
{
	return pmemkv_config_put_uint64(config, "force_create", value ? 1 : 0);
}

int pmemkv_config_put_comparator(pmemkv_config *config, pmemkv_comparator *comparator)
{
	return pmemkv_config_put_object(config, "comparator", comparator,
					(void (*)(void *)) & pmemkv_comparator_delete);
}

int pmemkv_config_put_oid(pmemkv_config *config, PMEMoid *oid)
{
	return pmemkv_config_put_object(config, "oid", oid, NULL);
}

int pmemkv_config_get_data(pmemkv_config *config, const char *key, const void **value,
			   size_t *value_size)
{
	if (!config)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_data(key, value, value_size)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

int pmemkv_config_get_object(pmemkv_config *config, const char *key, void **value)
{
	if (!config)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_object(key, value)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

int pmemkv_config_get_int64(pmemkv_config *config, const char *key, int64_t *value)
{
	if (!config)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_int64(key, value)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

int pmemkv_config_get_uint64(pmemkv_config *config, const char *key, uint64_t *value)
{
	if (!config)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_uint64(key, value)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

int pmemkv_config_get_string(pmemkv_config *config, const char *key, const char **value)
{
	if (!config)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_string(key, value)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

pmemkv_comparator *pmemkv_comparator_new(pmemkv_compare_function *fn, const char *name,
					 void *arg)
{
	if (!fn || !name) {
		ERR() << "comparison function and name must not be NULL";
		return nullptr;
	}

	try {
		return comparator_from_internal(
			new pmem::kv::internal::comparator(fn, name, arg));
	} catch (const std::exception &exc) {
		ERR() << exc.what();
		return nullptr;
	} catch (...) {
		ERR() << "Unspecified failure";
		return nullptr;
	}
}

void pmemkv_comparator_delete(pmemkv_comparator *comparator)
{
	try {
		delete comparator_to_internal(comparator);
	} catch (const std::exception &exc) {
		ERR() << exc.what();
	} catch (...) {
		ERR() << "Unspecified failure";
	}
}

int pmemkv_open(const char *engine_c_str, pmemkv_config *config, pmemkv_db **db)
{
	std::unique_ptr<pmem::kv::internal::config> cfg(config_to_internal(config));

	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		auto engine = pmem::kv::engine_base::create_engine(engine_c_str,
								   std::move(cfg));

		*db = db_from_internal(engine.release());

		return PMEMKV_STATUS_OK;
	});
}

void pmemkv_close(pmemkv_db *db)
{
	try {
		delete db_to_internal(db);
	} catch (const std::exception &exc) {
		ERR() << exc.what();
	} catch (...) {
		ERR() << "Unspecified failure";
	}
}

int pmemkv_count_all(pmemkv_db *db, size_t *cnt)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(
		__func__, [&] { return db_to_internal(db)->count_all(*cnt); });
}

int pmemkv_count_above(pmemkv_db *db, const char *k, size_t kb, size_t *cnt)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->count_above(pmem::kv::string_view(k, kb),
						       *cnt);
	});
}

int pmemkv_count_equal_above(pmemkv_db *db, const char *k, size_t kb, size_t *cnt)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->count_equal_above(pmem::kv::string_view(k, kb),
							     *cnt);
	});
}

int pmemkv_count_equal_below(pmemkv_db *db, const char *k, size_t kb, size_t *cnt)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->count_equal_below(pmem::kv::string_view(k, kb),
							     *cnt);
	});
}

int pmemkv_count_below(pmemkv_db *db, const char *k, size_t kb, size_t *cnt)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->count_below(pmem::kv::string_view(k, kb),
						       *cnt);
	});
}

int pmemkv_count_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
			 size_t kb2, size_t *cnt)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->count_between(pmem::kv::string_view(k1, kb1),
							 pmem::kv::string_view(k2, kb2),
							 *cnt);
	});
}

int pmemkv_get_all(pmemkv_db *db, pmemkv_get_kv_callback *c, void *arg)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(
		__func__, [&] { return db_to_internal(db)->get_all(c, arg); });
}

int pmemkv_get_above(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c,
		     void *arg)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->get_above(pmem::kv::string_view(k, kb), c,
						     arg);
	});
}

int pmemkv_get_equal_above(pmemkv_db *db, const char *k, size_t kb,
			   pmemkv_get_kv_callback *c, void *arg)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->get_equal_above(pmem::kv::string_view(k, kb),
							   c, arg);
	});
}

int pmemkv_get_equal_below(pmemkv_db *db, const char *k, size_t kb,
			   pmemkv_get_kv_callback *c, void *arg)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->get_equal_below(pmem::kv::string_view(k, kb),
							   c, arg);
	});
}

int pmemkv_get_below(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c,
		     void *arg)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->get_below(pmem::kv::string_view(k, kb), c,
						     arg);
	});
}

int pmemkv_get_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
		       size_t kb2, pmemkv_get_kv_callback *c, void *arg)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->get_between(pmem::kv::string_view(k1, kb1),
						       pmem::kv::string_view(k2, kb2), c,
						       arg);
	});
}

int pmemkv_exists(pmemkv_db *db, const char *k, size_t kb)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->exists(pmem::kv::string_view(k, kb));
	});
}

int pmemkv_get(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_v_callback *c,
	       void *arg)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->get(pmem::kv::string_view(k, kb), c, arg);
	});
}

struct GetCopyCallbackContext {
	int result;

	size_t buffer_size;
	char *buffer;

	size_t *value_size;
};

static void get_copy_callback(const char *v, size_t vb, void *arg)
{
	const auto c = ((GetCopyCallbackContext *)arg);

	if (c->value_size != nullptr)
		*(c->value_size) = vb;

	if (vb <= c->buffer_size) {
		c->result = PMEMKV_STATUS_OK;
		if (c->buffer != nullptr)
			memcpy(c->buffer, v, vb);
	} else {
		c->result = PMEMKV_STATUS_OUT_OF_MEMORY;
	}
}

int pmemkv_get_copy(pmemkv_db *db, const char *k, size_t kb, char *buffer,
		    size_t buffer_size, size_t *value_size)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	GetCopyCallbackContext ctx = {PMEMKV_STATUS_NOT_FOUND, buffer_size, buffer,
				      value_size};

	if (buffer != nullptr)
		memset(buffer, 0, buffer_size);

	auto ret = catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->get(pmem::kv::string_view(k, kb),
					       &get_copy_callback, &ctx);
	});

	if (ret != PMEMKV_STATUS_OK)
		return ret;

	return ctx.result;
}

int pmemkv_put(pmemkv_db *db, const char *k, size_t kb, const char *v, size_t vb)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->put(pmem::kv::string_view(k, kb),
					       pmem::kv::string_view(v, vb));
	});
}

int pmemkv_remove(pmemkv_db *db, const char *k, size_t kb)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->remove(pmem::kv::string_view(k, kb));
	});
}

int pmemkv_defrag(pmemkv_db *db, double start_percent, double amount_percent)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->defrag(start_percent, amount_percent);
	});
}

int pmemkv_stats(pmemkv_db *db, pmemkv_statistics *statistics)
{
	if (!db)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->stats(statistics);
	});
}

const char *pmemkv_errormsg(void)
{
	return out_get_errormsg();
}

} /* extern "C" */
