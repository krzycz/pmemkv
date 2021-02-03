// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.h"
#include <libpmemkv.h>

#include <libpmemobj/pool_base.h>
#include <stdlib.h>
#include <string.h>

/**
 * Tests all config methods using C API
 */

static const int TEST_VAL = 0xABC;
static const int INIT_VAL = 1;
static const int DELETED_VAL = 2;
static const char *PATH = "/some/path";
static const uint64_t SIZE = 0xDEADBEEF;

struct custom_type {
	int a;
	char b;
};

struct custom_type_wrapper {
	struct custom_type value;
	int additional_state;
};

static void *getter(void *arg)
{
	struct custom_type_wrapper *ct = (struct custom_type_wrapper *)(arg);
	return &ct->value;
}

static void deleter(struct custom_type *ct_ptr)
{
	ct_ptr->a = DELETED_VAL;
	ct_ptr->b = DELETED_VAL;
}

static void xdeleter(struct custom_type_wrapper *ct_ptr)
{
	ct_ptr->value.a = DELETED_VAL;
	ct_ptr->value.b = DELETED_VAL;
	ct_ptr->additional_state = DELETED_VAL;
}

static void simple_test()
{
	/**
	 * TEST: add and read data from config, using all available functions
	 */
	pmemkv_config *config = pmemkv_config_new();
	UT_ASSERT(config != NULL);

	int ret = pmemkv_config_put_string(config, "string", "abc");
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_int64(config, "int", 123);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	struct custom_type *ptr = malloc(sizeof(struct custom_type));
	ptr->a = INIT_VAL;
	ptr->b = INIT_VAL;
	ret = pmemkv_config_put_object(config, "object_ptr", ptr, NULL);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_data(config, "object", ptr, sizeof(*ptr));
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	struct custom_type *ptr_deleter = malloc(sizeof(struct custom_type));
	ptr_deleter->a = INIT_VAL;
	ptr_deleter->b = INIT_VAL;
	ret = pmemkv_config_put_object(config, "object_ptr_with_deleter", ptr_deleter,
				       (void (*)(void *)) & deleter);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_path(config, PATH);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_size(config, SIZE);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_force_create(config, true);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_reserve(config, 128);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	const char *value_string;
	ret = pmemkv_config_get_string(config, "string", &value_string);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERT(strcmp(value_string, "abc") == 0);

	int64_t value_int;
	ret = pmemkv_config_get_int64(config, "int", &value_int);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(value_int, 123);

	struct custom_type *value_custom_ptr;
	ret = pmemkv_config_get_object(config, "object_ptr", (void **)&value_custom_ptr);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(value_custom_ptr->a, INIT_VAL);
	UT_ASSERTeq(value_custom_ptr->b, INIT_VAL);

	struct custom_type *value_custom_ptr_deleter;
	ret = pmemkv_config_get_object(config, "object_ptr_with_deleter",
				       (void **)&value_custom_ptr_deleter);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(value_custom_ptr_deleter->a, INIT_VAL);
	UT_ASSERTeq(value_custom_ptr_deleter->b, INIT_VAL);

	struct custom_type *value_custom;
	size_t value_custom_size;
	ret = pmemkv_config_get_data(config, "object", (const void **)&value_custom,
				     &value_custom_size);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(value_custom_size, sizeof(value_custom));
	UT_ASSERTeq(value_custom->a, INIT_VAL);
	UT_ASSERTeq(value_custom->b, INIT_VAL);

	int64_t none;
	UT_ASSERTeq(pmemkv_config_get_int64(config, "non-existent", &none),
		    PMEMKV_STATUS_NOT_FOUND);

	ret = pmemkv_config_get_string(config, "path", &value_string);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERT(strcmp(value_string, PATH) == 0);

	uint64_t value_uint;
	ret = pmemkv_config_get_uint64(config, "size", &value_uint);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(value_uint, SIZE);

	ret = pmemkv_config_get_uint64(config, "force_create", &value_uint);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(value_uint, 1);

	ret = pmemkv_config_get_uint64(config, "reserve", &value_uint);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(value_uint, 128);

	pmemkv_config_delete(config);
	config = NULL;

	UT_ASSERTeq(value_custom_ptr_deleter->a, DELETED_VAL);
	UT_ASSERTeq(value_custom_ptr_deleter->b, DELETED_VAL);

	/* deleter was not set */
	UT_ASSERTeq(ptr, value_custom_ptr);
	UT_ASSERTeq(value_custom_ptr->a, INIT_VAL);
	UT_ASSERTeq(value_custom_ptr->b, INIT_VAL);

	free(ptr);
	free(ptr_deleter);
}

static void put_oid_simple_test()
{
	/**
	 * TEST: basic check for put_oid function.
	 */
	pmemkv_config *cfg = pmemkv_config_new();
	UT_ASSERT(cfg != NULL);
	PMEMoid oid;
	int ret = pmemkv_config_put_oid(cfg, &oid);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	PMEMoid *oid_ptr;
	ret = pmemkv_config_get_object(cfg, "oid", (void **)&oid_ptr);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(&oid, oid_ptr);

	pmemkv_config_delete(cfg);
}

static void free_deleter_test()
{
	/**
	 * TEST: checks if put_object will work with 'free' command as deleter
	 */
	pmemkv_config *config = pmemkv_config_new();
	UT_ASSERT(config != NULL);

	struct custom_type *ptr = malloc(sizeof(struct custom_type));
	ptr->a = INIT_VAL;
	ptr->b = INIT_VAL;
	int ret = pmemkv_config_put_object(config, "object_ptr", ptr, free);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	pmemkv_config_delete(config);
}

static void ex_put_object_test()
{
	/**
	 * TEST: checks if put_object_cb's deleter with additional state is working
	 * properly
	 */
	pmemkv_config *config = pmemkv_config_new();
	UT_ASSERT(config != NULL);

	struct custom_type_wrapper *ptr = malloc(sizeof(struct custom_type_wrapper));
	ptr->value.a = INIT_VAL;
	ptr->value.b = INIT_VAL;
	ptr->additional_state = TEST_VAL;
	int ret = pmemkv_config_put_object_cb(config, "object_ptr", ptr,
					      (void *(*)(void *))getter,
					      (void (*)(void *))xdeleter);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	struct custom_type *ptr_from_get;
	pmemkv_config_get_object(config, "object_ptr", (void **)&ptr_from_get);
	UT_ASSERTeq(ptr_from_get->a, INIT_VAL);
	UT_ASSERTeq(ptr_from_get->b, INIT_VAL);

	pmemkv_config_delete(config);
	config = NULL;

	UT_ASSERTeq(ptr->value.a, DELETED_VAL);
	UT_ASSERTeq(ptr->value.b, DELETED_VAL);
	UT_ASSERTeq(ptr->additional_state, DELETED_VAL);

	free(ptr);
}

static void ex_put_object_nullptr_del_test()
{
	/**
	 * TEST: checs if put_object_cb will work with nullptr deleter
	 */
	pmemkv_config *config = pmemkv_config_new();
	UT_ASSERT(config != NULL);

	struct custom_type_wrapper *ptr = malloc(sizeof(struct custom_type_wrapper));
	ptr->value.a = INIT_VAL;
	ptr->value.b = INIT_VAL;
	ptr->additional_state = TEST_VAL;
	int ret = pmemkv_config_put_object_cb(config, "object_ptr", ptr,
					      (void *(*)(void *))getter, NULL);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	pmemkv_config_delete(config);
	config = NULL;

	UT_ASSERTeq(ptr->value.a, INIT_VAL);
	UT_ASSERTeq(ptr->value.b, INIT_VAL);
	UT_ASSERTeq(ptr->additional_state, TEST_VAL);

	free(ptr);
}

static void ex_put_object_nullptr_getter_test()
{
	/**
	 * TEST: put_object_cb should not work with nullptr getter function
	 */
	pmemkv_config *config = pmemkv_config_new();
	UT_ASSERT(config != NULL);

	struct custom_type_wrapper *ptr = malloc(sizeof(struct custom_type_wrapper));
	ptr->value.a = INIT_VAL;
	ptr->value.b = INIT_VAL;
	ptr->additional_state = TEST_VAL;
	int ret = pmemkv_config_put_object_cb(config, "object_ptr", ptr, NULL, NULL);
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	pmemkv_config_delete(config);
	free(ptr);
}

static void ex_put_object_free_del_test()
{
	/**
	 * TEST: checks if put_object_cb will work with 'free' command as deleter
	 */
	pmemkv_config *config = pmemkv_config_new();
	UT_ASSERT(config != NULL);

	struct custom_type_wrapper *ptr = malloc(sizeof(struct custom_type_wrapper));
	ptr->value.a = INIT_VAL;
	ptr->value.b = INIT_VAL;
	ptr->additional_state = TEST_VAL;
	int ret = pmemkv_config_put_object_cb(config, "object_ptr", ptr,
					      (void *(*)(void *))getter, free);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	pmemkv_config_delete(config);
	config = NULL;
}

static void integral_conversion_test()
{
	/**
	 * TEST: when reading data from config it's allowed to read integers
	 * into different type (then it was originally stored), as long as
	 * the conversion is possible. CONFIG_TYPE_ERROR should be returned
	 * when e.g. reading negative integral value into signed int type.
	 */
	pmemkv_config *config = pmemkv_config_new();
	UT_ASSERT(config != NULL);

	int ret = pmemkv_config_put_int64(config, "int", 123);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_uint64(config, "uint", 123);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_int64(config, "negative-int", -123);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_uint64(config, "uint-max", (uint64_t)-1);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	int64_t int_s;
	ret = pmemkv_config_get_int64(config, "int", &int_s);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(int_s, 123);

	size_t int_us;
	ret = pmemkv_config_get_uint64(config, "int", &int_us);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(int_us, 123U);

	int64_t uint_s;
	ret = pmemkv_config_get_int64(config, "uint", &uint_s);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(uint_s, 123);

	size_t uint_us;
	ret = pmemkv_config_get_uint64(config, "uint", &uint_us);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(uint_us, 123U);

	int64_t neg_int_s;
	ret = pmemkv_config_get_int64(config, "negative-int", &neg_int_s);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(neg_int_s, -123);

	size_t neg_int_us;
	ret = pmemkv_config_get_uint64(config, "negative-int", &neg_int_us);
	UT_ASSERTeq(ret, PMEMKV_STATUS_CONFIG_TYPE_ERROR);

	int64_t uint_max_s;
	ret = pmemkv_config_get_int64(config, "uint-max", &uint_max_s);
	UT_ASSERTeq(ret, PMEMKV_STATUS_CONFIG_TYPE_ERROR);

	size_t uint_max_us;
	ret = pmemkv_config_get_uint64(config, "uint-max", &uint_max_us);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(uint_max_us, ((uint64_t)-1));

	pmemkv_config_delete(config);
}

static void not_found_test()
{
	/**
	 * TEST: all config get_* methods should return status NOT_FOUND if item
	 * does not exist
	 */
	pmemkv_config *config = pmemkv_config_new();
	UT_ASSERT(config != NULL);

	const char *my_string;
	int ret = pmemkv_config_get_string(config, "non-existent-string", &my_string);
	UT_ASSERTeq(ret, PMEMKV_STATUS_NOT_FOUND);

	int64_t my_int;
	ret = pmemkv_config_get_int64(config, "non-existent-int", &my_int);
	UT_ASSERTeq(ret, PMEMKV_STATUS_NOT_FOUND);

	size_t my_uint;
	ret = pmemkv_config_get_uint64(config, "non-existent-uint", &my_uint);
	UT_ASSERTeq(ret, PMEMKV_STATUS_NOT_FOUND);

	struct custom_type *my_object;
	ret = pmemkv_config_get_object(config, "non-existent-object",
				       (void **)&my_object);
	UT_ASSERTeq(ret, PMEMKV_STATUS_NOT_FOUND);

	size_t my_object_size = 0;
	ret = pmemkv_config_get_data(config, "non-existent-data",
				     (const void **)&my_object, &my_object_size);
	UT_ASSERTeq(ret, PMEMKV_STATUS_NOT_FOUND);
	UT_ASSERTeq(my_object_size, 0U);

	pmemkv_config_delete(config);
}

static void null_config_test()
{
	/**
	 * TEST: in C API all config methods require 'config' as param - it can't be null
	 */
	int ret = pmemkv_config_put_string(NULL, "string", "abc");
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	ret = pmemkv_config_put_int64(NULL, "int", 123);
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	ret = pmemkv_config_put_uint64(NULL, "uint", 123456);
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	struct custom_type *ptr = malloc(sizeof(struct custom_type));
	ptr->a = INIT_VAL;
	ptr->b = INIT_VAL;
	ret = pmemkv_config_put_object(NULL, "object_ptr", ptr, NULL);
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	ret = pmemkv_config_put_data(NULL, "object", ptr, sizeof(*ptr));
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	const char *value_string;
	ret = pmemkv_config_get_string(NULL, "string", &value_string);
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	int64_t value_int;
	ret = pmemkv_config_get_int64(NULL, "int", &value_int);
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	uint64_t value_uint;
	ret = pmemkv_config_get_uint64(NULL, "uint", &value_uint);
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	struct custom_type *value_custom_ptr;
	ret = pmemkv_config_get_object(NULL, "object_ptr", (void **)&value_custom_ptr);
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	struct custom_type *value_custom;
	size_t value_custom_size;
	ret = pmemkv_config_get_data(NULL, "object", (const void **)&value_custom,
				     &value_custom_size);
	UT_ASSERTeq(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	free(ptr);
}

int main(int argc, char *argv[])
{
	START();

	simple_test();
	put_oid_simple_test();
	free_deleter_test();
	ex_put_object_test();
	ex_put_object_nullptr_del_test();
	ex_put_object_free_del_test();
	ex_put_object_nullptr_getter_test();
	integral_conversion_test();
	not_found_test();
	null_config_test();
}
