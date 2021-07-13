// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

#include <ctime>
#include <limits.h>
#include <set>

using namespace pmem::kv;

static const std::string PREFIX = "in";
static const std::string SUFFIX = ";";
static const std::string CLEAN_KEY_SUFFIX = "_cl";

static constexpr size_t CHARSET_RANGE_START = 0;
static constexpr size_t CHARSET_RANGE_END = (size_t)UCHAR_MAX;
static constexpr size_t CHARSET_LEN = CHARSET_RANGE_END - CHARSET_RANGE_START + 1;

/* It generates a set that contains random strings of various length but no longer than
 * 'max_str_len'. */
std::set<std::string> generate_binary_strings(const size_t cnt, const size_t max_str_len)
{
	std::set<std::string> strings;
	size_t n = 0;
	do {
		std::string gen_str = std::to_string(n);
		/* various length of string, min: len of gen_str */
		size_t rand_len = (size_t)rand() % (max_str_len - gen_str.size());

		for (size_t i = 0; i < rand_len; i++) {
			gen_str.push_back(
				char(CHARSET_RANGE_START + (size_t)rand() % CHARSET_LEN));
		}
		if (strings.insert(gen_str).second)
			++n;
	} while (n != cnt);

	return strings;
}

static void BinaryKeyTest(pmem::kv::db &kv)
{
	/**
	 * TEST: each char from the char range is used in two keys
	 * once with prefix and suffix, once just as is ("clean key")
	 */
	size_t cnt = std::numeric_limits<size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);

	auto key = entry_from_string(PREFIX);
	auto value = entry_from_string("constval");
	ASSERT_STATUS(kv.exists(key), status::NOT_FOUND);

	ASSERT_STATUS(kv.put(key, value), status::OK);
	ASSERT_STATUS(kv.exists(key), status::OK);
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);

	/* Add binary keys */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		/* key with prefix and suffix */
		std::string key1 =
			entry_from_string(std::string(PREFIX + char(i) + SUFFIX));
		ASSERT_STATUS(kv.exists(key1), status::NOT_FOUND);
		ASSERT_STATUS(kv.put(key1, entry_from_number(i)), status::OK);

		/* "clean" key */
		std::string key2 = entry_from_string(std::string(1, char(i)));
		ASSERT_STATUS(kv.exists(key2), status::NOT_FOUND);
		ASSERT_STATUS(kv.put(key2, entry_from_number(i)), status::OK);
	}

	std::string val;
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == (CHARSET_LEN * 2 + 1));
	ASSERT_STATUS(kv.get(key, &val), status::OK);
	UT_ASSERT(val == value);

	/* Read and remove binary keys */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		/* key with prefix and suffix */
		std::string key1 =
			entry_from_string(std::string(PREFIX + char(i) + SUFFIX));
		ASSERT_STATUS(kv.exists(key1), status::OK);
		ASSERT_STATUS(kv.get(key1, &val), status::OK);
		UT_ASSERT(val == entry_from_number(i));

		ASSERT_STATUS(kv.remove(key1), status::OK);
		ASSERT_STATUS(kv.exists(key1), status::NOT_FOUND);

		/* "clean" key */
		std::string key2 = entry_from_string(std::string(1, char(i)));
		ASSERT_STATUS(kv.exists(key2), status::OK);
		ASSERT_STATUS(kv.get(key2, &val), status::OK);
		UT_ASSERT(val == entry_from_number(i));

		ASSERT_STATUS(kv.remove(key2), status::OK);
		ASSERT_STATUS(kv.exists(key2), status::NOT_FOUND);
	}

	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.get(key, &val), status::OK);
	UT_ASSERT(val == value);
	ASSERT_STATUS(kv.remove(key), status::OK);
}

static void BinaryRandKeyTest(const size_t elements_cnt, const size_t max_key_len,
			      pmem::kv::db &kv)
{
	/**
	 * TEST: keys are randomly generated from the full range of the charset
	 */
	size_t cnt = std::numeric_limits<size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);

	auto keys = generate_binary_strings(elements_cnt, max_key_len);

	/* Add elements with generated keys */
	size_t i = 0;
	for (auto &str : keys) {
		std::string istr = entry_from_number(i++);
		std::string key = entry_from_string(str);
		ASSERT_STATUS(kv.put(key, istr), status::OK);
	}

	std::string value;
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == elements_cnt);

	/* Read and remove elements with generated keys (in reverse order) */
	i = cnt;
	for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
		std::string istr = entry_from_number(--i);
		std::string key = entry_from_string(*it);

		ASSERT_STATUS(kv.exists(key), status::OK);
		ASSERT_STATUS(kv.get(key, &value), status::OK);
		UT_ASSERT(value == istr);

		ASSERT_STATUS(kv.remove(key), status::OK);
		ASSERT_STATUS(kv.exists(key), status::NOT_FOUND);
	}
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
}

static void BinaryValueTest(pmem::kv::db &kv)
{
	/**
	 * TEST: each char from the char range is used as value twice -
	 * once with prefix and suffix, once just as is ("clean value")
	 */
	size_t cnt = std::numeric_limits<size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);

	/* Add elements with binary values */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		/* value with prefix and suffix */
		std::string key1 = entry_from_number(i);
		std::string value1 =
			entry_from_string(std::string(PREFIX + char(i) + SUFFIX));
		ASSERT_STATUS(kv.exists(key1), status::NOT_FOUND);
		ASSERT_STATUS(kv.put(key1, value1), status::OK);

		/* "clean" value */
		std::string key2 = entry_from_number(i, "", CLEAN_KEY_SUFFIX);
		std::string value2 = entry_from_string(std::string(1, char(i)));
		ASSERT_STATUS(kv.exists(key2), status::NOT_FOUND);
		ASSERT_STATUS(kv.put(key2, value2), status::OK);
	}

	std::string value;
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == (CHARSET_LEN * 2));

	/* Read and remove elements with binary values */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		/* value with prefix and suffix */
		std::string key1 = entry_from_number(i);
		std::string value1 =
			entry_from_string(std::string(PREFIX + char(i) + SUFFIX));

		ASSERT_STATUS(kv.exists(key1), status::OK);
		ASSERT_STATUS(kv.get(key1, &value), status::OK);
		UT_ASSERT(value == value1);
		UT_ASSERTeq(value.length(), value1.length());
		ASSERT_STATUS(kv.remove(key1), status::OK);
		ASSERT_STATUS(kv.exists(key1), status::NOT_FOUND);

		/* "clean" value */
		std::string key2 = entry_from_number(i, "", CLEAN_KEY_SUFFIX);
		std::string value2 = entry_from_string(std::string(1, char(i)));

		ASSERT_STATUS(kv.exists(key2), status::OK);
		ASSERT_STATUS(kv.get(key2, &value), status::OK);
		UT_ASSERT(value == value2);
		UT_ASSERTeq(value.length(), value2.length());
		ASSERT_STATUS(kv.remove(key2), status::OK);
		ASSERT_STATUS(kv.exists(key2), status::NOT_FOUND);
	}

	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
}

static void BinaryRandValueTest(const size_t elements_cnt, const size_t max_value_len,
				pmem::kv::db &kv)
{
	/**
	 * TEST: values are randomly generated from the full range of the charset
	 */
	size_t cnt = std::numeric_limits<size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);

	auto values = generate_binary_strings(elements_cnt, max_value_len);

	/* Add elements with generated values */
	size_t i = 0;
	for (auto &str : values) {
		std::string key = entry_from_number(i++);
		std::string value = entry_from_string(str);

		ASSERT_STATUS(kv.put(key, value), status::OK);
	}

	std::string value;
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == elements_cnt);

	/* Read and remove elements with generated values (in reverse order) */
	i = cnt;
	for (auto it = values.rbegin(); it != values.rend(); ++it) {
		std::string key = entry_from_number(--i);
		std::string exp_value = entry_from_string(*it);

		ASSERT_STATUS(kv.get(key, &value), status::OK);
		UT_ASSERT(value == exp_value);
		UT_ASSERTeq(value.length(), exp_value.length());
		ASSERT_STATUS(kv.remove(key), status::OK);
	}

	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 5)
		UT_FATAL("usage: %s engine json_config elements_cnt max_str_len",
			 argv[0]);

	auto seed = unsigned(std::time(0));
	printf("rand seed: %u\n", seed);
	std::srand(seed);

	auto elements_cnt = std::stoull(argv[3]);
	auto max_str_len = std::stoull(argv[4]);
	run_engine_tests(
		argv[1], argv[2],
		{
			BinaryKeyTest,
			std::bind(BinaryRandKeyTest, elements_cnt, max_str_len, _1),
			BinaryValueTest,
			std::bind(BinaryRandValueTest, elements_cnt, max_str_len, _1),
		});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
