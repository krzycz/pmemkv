// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * pmemkv_benchmark.c -- benchmark usage of fmap engine.
 */

#include <assert.h>
#include <libpmemkv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			puts(pmemkv_errormsg());                                         \
		assert(expr);                                                            \
	} while (0)

#define LOG(msg) puts(msg)
#define MAX_KEY_LEN 16
#define MAX_VAL_LEN 1024
#define MAX_BEN_ITEM 10000000
#define MAX_INTERVAL_TIMES 1000

static const uint64_t SIZE = 16 * 1024UL * 1024UL * 1024UL;

int get_kv_callback(const char *k, size_t kb, const char *value, size_t value_bytes,
		    void *arg)
{
	printf("   visited: %s\n", k);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		exit(1);
	}

	/* See libpmemkv_config(3) for more detailed example of config creation */
	LOG("Creating config");
	pmemkv_config *cfg = pmemkv_config_new();
	ASSERT(cfg != NULL);

	int s = pmemkv_config_put_path(cfg, argv[1]);
	ASSERT(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put_size(cfg, SIZE);
	ASSERT(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put_force_create(cfg, true);
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Opening pmemkv database with 'cmap' engine");
	pmemkv_db *db = NULL;
	s = pmemkv_open("cmap", cfg, &db);
	ASSERT(s == PMEMKV_STATUS_OK);
	ASSERT(db != NULL);

	LOG("Starting benchmarking...");
	char curkey[MAX_KEY_LEN];
	uint8_t curval[MAX_VAL_LEN];
	for (int j = 0; j < MAX_VAL_LEN; j++) {
		curval[j] = (uint8_t)((j+1) % 255);
	}

	long long cur_us;
	long long last_us = 0;
	struct timeval now;
	gettimeofday(&now, NULL);
	last_us = (long long)(now.tv_sec * 1000000 + now.tv_usec);

	for (int i = 0, j = 0; i < MAX_BEN_ITEM; i++) {
		snprintf(curkey, sizeof(curkey), "key%12d:", i);
		s = pmemkv_put(db, curkey, strlen(curkey), curval, MAX_VAL_LEN);
		ASSERT(s == PMEMKV_STATUS_OK);
		if (++j % MAX_INTERVAL_TIMES == 0) {
			gettimeofday(&now, NULL);
			cur_us = (long long)(now.tv_sec * 1000000 + now.tv_usec);

			double inst_dt = (double)(cur_us - last_us);
			double inst_ops = (double)MAX_INTERVAL_TIMES/inst_dt * 1000000.0;
			printf("WRITE: %.2f ops\n", inst_ops);
			last_us = cur_us;
		}
	}

	LOG("\nClosing database");
	pmemkv_close(db);

	return 0;
}
