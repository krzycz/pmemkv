/*
 * Copyright 2017-2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <sys/stat.h>

#include "config.h"
#include "engine.h"
#include "engines/blackhole.h"
#include "exceptions.h"
#include "libpmemkv.h"
#include "libpmemkv.hpp"
#include "out.h"

#ifdef ENGINE_VSMAP
#include "engines/vsmap.h"
#endif

#ifdef ENGINE_VCMAP
#include "engines/vcmap.h"
#endif

#ifdef ENGINE_CMAP
#include "engines/cmap.h"
#endif

#ifdef ENGINE_CACHING
#include "engines-experimental/caching.h"
#endif

#ifdef ENGINE_STREE
#include "engines-experimental/stree.h"
#endif

#ifdef ENGINE_TREE3
#include "engines-experimental/tree3.h"
#endif

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

static inline pmemkv_db *db_from_internal(pmem::kv::engine_base *engine)
{
	return reinterpret_cast<pmemkv_db *>(engine);
}

static inline pmem::kv::engine_base *db_to_internal(pmemkv_db *db)
{
	return reinterpret_cast<pmem::kv::engine_base *>(db);
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
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		out_err_stream(func_name) << "Unspecified error";
		return PMEMKV_STATUS_FAILED;
	}
}

extern "C" {

pmemkv_config *pmemkv_config_new(void)
{
	try {
		return reinterpret_cast<pmemkv_config *>(new pmem::kv::internal::config);
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
		delete reinterpret_cast<pmem::kv::internal::config *>(config);
	} catch (const std::exception &exc) {
		ERR() << exc.what();
	} catch (...) {
		ERR() << "Unspecified failure";
	}
}

int pmemkv_config_from_json(pmemkv_config *config, const char *json)
{
	rapidjson::Document doc;
	rapidjson::Value::ConstMemberIterator itr;

	assert(config && json);

	try {
		if (doc.Parse(json).HasParseError())
			return PMEMKV_STATUS_CONFIG_PARSING_ERROR;

		for (itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
			if (itr->value.IsString()) {
				auto value = itr->value.GetString();

				auto status = pmemkv_config_put_string(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting string to the config failed");
			} else if (itr->value.IsInt64()) {
				auto value = itr->value.GetInt64();

				auto status = pmemkv_config_put_int64(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting int to the config failed");
			} else if (itr->value.IsDouble()) {
				auto value = itr->value.GetDouble();

				auto status = pmemkv_config_put_double(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting double to the config failed");
			} else if (itr->value.IsTrue() || itr->value.IsFalse()) {
				auto value = itr->value.GetBool();

				auto status = pmemkv_config_put_int64(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting bool to the config failed");
			} else if (itr->value.IsObject()) {
				rapidjson::StringBuffer sb;
				rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
				itr->value.Accept(writer);

				auto sub_cfg = pmemkv_config_new();

				if (sub_cfg == nullptr) {
					ERR() << "Cannot allocate subconfig";
					return PMEMKV_STATUS_FAILED;
				}

				auto status =
					pmemkv_config_from_json(sub_cfg, sb.GetString());
				if (status != PMEMKV_STATUS_OK) {
					pmemkv_config_delete(sub_cfg);
					throw std::runtime_error(
						"Cannot parse subconfig");
				}

				status = pmemkv_config_put_object(
					config, itr->name.GetString(), sub_cfg,
					(void (*)(void *)) & pmemkv_config_delete);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting a new entry to the config failed");
			} else {
				static std::string kTypeNames[] = {
					"Null",  "False",  "True",  "Object",
					"Array", "String", "Number"};

				throw std::runtime_error(
					"Unsupported data type in JSON string: " +
					kTypeNames[itr->value.GetType()]);
			}
		}
	} catch (const std::exception &exc) {
		ERR() << exc.what();
		return PMEMKV_STATUS_CONFIG_PARSING_ERROR;
	} catch (...) {
		ERR() << "Unspecified failure";
		return PMEMKV_STATUS_CONFIG_PARSING_ERROR;
	}

	return PMEMKV_STATUS_OK;
}

int pmemkv_config_put_data(pmemkv_config *config, const char *key, const void *value,
			   size_t value_size)
{
	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_data(key, value, value_size);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_object(pmemkv_config *config, const char *key, void *value,
			     void (*deleter)(void *))
{
	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_object(key, value, deleter);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_int64(pmemkv_config *config, const char *key, int64_t value)
{
	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_int64(key, value);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_uint64(pmemkv_config *config, const char *key, uint64_t value)
{
	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_uint64(key, value);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_double(pmemkv_config *config, const char *key, double value)
{
	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_double(key, value);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_put_string(pmemkv_config *config, const char *key, const char *value)
{
	return catch_and_return_status(__func__, [&] {
		config_to_internal(config)->put_string(key, value);
		return PMEMKV_STATUS_OK;
	});
}

int pmemkv_config_get_data(pmemkv_config *config, const char *key, const void **value,
			   size_t *value_size)
{
	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_data(key, value, value_size)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

int pmemkv_config_get_object(pmemkv_config *config, const char *key, void **value)
{
	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_object(key, value)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

int pmemkv_config_get_int64(pmemkv_config *config, const char *key, int64_t *value)
{
	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_int64(key, value)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

int pmemkv_config_get_uint64(pmemkv_config *config, const char *key, uint64_t *value)
{
	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_uint64(key, value)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

int pmemkv_config_get_double(pmemkv_config *config, const char *key, double *value)
{
	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_double(key, value)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

int pmemkv_config_get_string(pmemkv_config *config, const char *key, const char **value)
{
	return catch_and_return_status(__func__, [&] {
		return config_to_internal(config)->get_string(key, value)
			? PMEMKV_STATUS_OK
			: PMEMKV_STATUS_NOT_FOUND;
	});
}

int pmemkv_open(const char *engine_c_str, pmemkv_config *config, pmemkv_db **db)
{
	if (db == nullptr)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	try {
		std::string engine = engine_c_str;

		auto cfg = std::unique_ptr<pmem::kv::internal::config>(
			reinterpret_cast<pmem::kv::internal::config *>(config));

		if (engine == "blackhole") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::blackhole(std::move(cfg)));

			return PMEMKV_STATUS_OK;
		}
#ifdef ENGINE_CACHING
		if (engine == "caching") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::caching(std::move(cfg)));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_TREE3
		if (engine == "tree3") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::tree3(std::move(cfg)));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_STREE
		if (engine == "stree") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::stree(std::move(cfg)));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_CMAP
		if (engine == "cmap") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::cmap(std::move(cfg)));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_VSMAP
		if (engine == "vsmap") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::vsmap(std::move(cfg)));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_VCMAP
		if (engine == "vcmap") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::vcmap(std::move(cfg)));

			return PMEMKV_STATUS_OK;
		}
#endif
		throw std::runtime_error("Unknown engine name");
	} catch (std::exception &e) {
		ERR() << e.what();
		*db = nullptr;

		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR() << "Unspecified failure";
		return PMEMKV_STATUS_FAILED;
	}
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
	return catch_and_return_status(
		__func__, [&] { return db_to_internal(db)->count_all(*cnt); });
}

int pmemkv_count_above(pmemkv_db *db, const char *k, size_t kb, size_t *cnt)
{
	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->count_above(pmem::kv::string_view(k, kb),
						       *cnt);
	});
}

int pmemkv_count_below(pmemkv_db *db, const char *k, size_t kb, size_t *cnt)
{
	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->count_below(pmem::kv::string_view(k, kb),
						       *cnt);
	});
}

int pmemkv_count_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
			 size_t kb2, size_t *cnt)
{
	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->count_between(pmem::kv::string_view(k1, kb1),
							 pmem::kv::string_view(k2, kb2),
							 *cnt);
	});
}

int pmemkv_get_all(pmemkv_db *db, pmemkv_get_kv_callback *c, void *arg)
{
	return catch_and_return_status(
		__func__, [&] { return db_to_internal(db)->get_all(c, arg); });
}

int pmemkv_get_above(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c,
		     void *arg)
{
	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->get_above(pmem::kv::string_view(k, kb), c,
						     arg);
	});
}

int pmemkv_get_below(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c,
		     void *arg)
{
	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->get_below(pmem::kv::string_view(k, kb), c,
						     arg);
	});
}

int pmemkv_get_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
		       size_t kb2, pmemkv_get_kv_callback *c, void *arg)
{
	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->get_between(pmem::kv::string_view(k1, kb1),
						       pmem::kv::string_view(k2, kb2), c,
						       arg);
	});
}

int pmemkv_exists(pmemkv_db *db, const char *k, size_t kb)
{
	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->exists(pmem::kv::string_view(k, kb));
	});
}

int pmemkv_get(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_v_callback *c,
	       void *arg)
{
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

	if (vb < c->buffer_size) {
		c->result = PMEMKV_STATUS_OK;
		if (c->buffer != nullptr)
			memcpy(c->buffer, v, vb);
	} else {
		c->result = PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_get_copy(pmemkv_db *db, const char *k, size_t kb, char *buffer,
		    size_t buffer_size, size_t *value_size)
{
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
	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->put(pmem::kv::string_view(k, kb),
					       pmem::kv::string_view(v, vb));
	});
}

int pmemkv_remove(pmemkv_db *db, const char *k, size_t kb)
{
	return catch_and_return_status(__func__, [&] {
		return db_to_internal(db)->remove(pmem::kv::string_view(k, kb));
	});
}

const char *pmemkv_errormsg(void)
{
	return out_get_errormsg();
}

} /* extern "C" */
