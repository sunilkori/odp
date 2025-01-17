/* Copyright (c) 2015-2018, Linaro Limited
 * Copyright (c) 2019-2022, Nokia
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <time.h>

#include <odp_api.h>
#include "odp_cunit_common.h"

#define BUSY_LOOP_CNT		30000000    /* used for t > min resolution */
#define MIN_TIME_RATE		32000
#define MAX_TIME_RATE		15000000000
#define DELAY_TOLERANCE		40000000	    /* deviation for delay */
#define WAIT_SECONDS            3

static uint64_t local_res;
static uint64_t global_res;

typedef odp_time_t time_cb(void);
typedef uint64_t time_res_cb(void);
typedef odp_time_t time_from_ns_cb(uint64_t ns);
typedef uint64_t time_nsec_cb(void);

static void time_test_constants(void)
{
	uint64_t ns;

	CU_ASSERT(ODP_TIME_USEC_IN_NS == 1000);

	ns = ODP_TIME_HOUR_IN_NS;
	CU_ASSERT(ns == 60 * ODP_TIME_MIN_IN_NS);
	ns = ODP_TIME_MIN_IN_NS;
	CU_ASSERT(ns == 60 * ODP_TIME_SEC_IN_NS);
	ns = ODP_TIME_SEC_IN_NS;
	CU_ASSERT(ns == 1000 * ODP_TIME_MSEC_IN_NS);
	ns = ODP_TIME_MSEC_IN_NS;
	CU_ASSERT(ns == 1000 * ODP_TIME_USEC_IN_NS);

	ns = ODP_TIME_SEC_IN_NS / 1000;
	CU_ASSERT(ns == ODP_TIME_MSEC_IN_NS);
	ns /= 1000;
	CU_ASSERT(ns == ODP_TIME_USEC_IN_NS);
}

static void time_test_res(time_res_cb time_res, uint64_t *res)
{
	uint64_t rate;

	rate = time_res();
	CU_ASSERT(rate > MIN_TIME_RATE);
	CU_ASSERT(rate < MAX_TIME_RATE);

	*res = ODP_TIME_SEC_IN_NS / rate;
	if (ODP_TIME_SEC_IN_NS % rate)
		(*res)++;
}

static void time_test_local_res(void)
{
	time_test_res(odp_time_local_res, &local_res);
}

static void time_test_global_res(void)
{
	time_test_res(odp_time_global_res, &global_res);
}

/* check that related conversions come back to the same value */
static void time_test_conversion(time_from_ns_cb time_from_ns, uint64_t res)
{
	uint64_t ns1, ns2;
	odp_time_t time;
	uint64_t upper_limit, lower_limit;

	ns1 = 100;
	time = time_from_ns(ns1);

	ns2 = odp_time_to_ns(time);

	/* need to check within arithmetic tolerance that the same
	 * value in ns is returned after conversions */
	upper_limit = ns1 + res;
	lower_limit = ns1 - res;
	CU_ASSERT((ns2 <= upper_limit) && (ns2 >= lower_limit));

	ns1 = 60 * 11 * ODP_TIME_SEC_IN_NS;
	time = time_from_ns(ns1);

	ns2 = odp_time_to_ns(time);

	/* need to check within arithmetic tolerance that the same
	 * value in ns is returned after conversions */
	upper_limit = ns1 + res;
	lower_limit = ns1 - res;
	CU_ASSERT((ns2 <= upper_limit) && (ns2 >= lower_limit));

	/* test on 0 */
	ns1 = odp_time_to_ns(ODP_TIME_NULL);
	CU_ASSERT(ns1 == 0);
}

static void time_test_local_conversion(void)
{
	time_test_conversion(odp_time_local_from_ns, local_res);
}

static void time_test_global_conversion(void)
{
	time_test_conversion(odp_time_global_from_ns, global_res);
}

static void time_test_monotony(void)
{
	volatile uint64_t count = 0;
	odp_time_t l_t1, l_t2, l_t3;
	odp_time_t g_t1, g_t2, g_t3;
	uint64_t lns_t1, lns_t2, lns_t3;
	uint64_t gns_t1, gns_t2, gns_t3;
	uint64_t ns1, ns2, ns3;

	l_t1 = odp_time_local();
	g_t1 = odp_time_global();
	lns_t1 = odp_time_local_ns();
	gns_t1 = odp_time_global_ns();

	while (count < BUSY_LOOP_CNT) {
		count++;
	};

	l_t2 = odp_time_local();
	g_t2 = odp_time_global();
	lns_t2 = odp_time_local_ns();
	gns_t2 = odp_time_global_ns();

	while (count < BUSY_LOOP_CNT) {
		count++;
	};

	l_t3 = odp_time_local();
	g_t3 = odp_time_global();
	lns_t3 = odp_time_local_ns();
	gns_t3 = odp_time_global_ns();

	ns1 = odp_time_to_ns(l_t1);
	ns2 = odp_time_to_ns(l_t2);
	ns3 = odp_time_to_ns(l_t3);

	/* Local time assertions */
	CU_ASSERT(ns2 > ns1);
	CU_ASSERT(ns3 > ns2);

	ns1 = odp_time_to_ns(g_t1);
	ns2 = odp_time_to_ns(g_t2);
	ns3 = odp_time_to_ns(g_t3);

	/* Global time assertions */
	CU_ASSERT(ns2 > ns1);
	CU_ASSERT(ns3 > ns2);

	/* Local time in nsec */
	CU_ASSERT(lns_t2 > lns_t1);
	CU_ASSERT(lns_t3 > lns_t2);

	/* Global time in nsec */
	CU_ASSERT(gns_t2 > gns_t1);
	CU_ASSERT(gns_t3 > gns_t2);
}

static void time_test_cmp(time_cb time_cur, time_from_ns_cb time_from_ns)
{
	/* volatile to stop optimization of busy loop */
	volatile int count = 0;
	odp_time_t t1, t2, t3;

	t1 = time_cur();

	while (count < BUSY_LOOP_CNT) {
		count++;
	};

	t2 = time_cur();

	while (count < BUSY_LOOP_CNT * 2) {
		count++;
	};

	t3 = time_cur();

	CU_ASSERT(odp_time_cmp(t2, t1) > 0);
	CU_ASSERT(odp_time_cmp(t3, t2) > 0);
	CU_ASSERT(odp_time_cmp(t3, t1) > 0);
	CU_ASSERT(odp_time_cmp(t1, t2) < 0);
	CU_ASSERT(odp_time_cmp(t2, t3) < 0);
	CU_ASSERT(odp_time_cmp(t1, t3) < 0);
	CU_ASSERT(odp_time_cmp(t1, t1) == 0);
	CU_ASSERT(odp_time_cmp(t2, t2) == 0);
	CU_ASSERT(odp_time_cmp(t3, t3) == 0);

	t2 = time_from_ns(60 * 10 * ODP_TIME_SEC_IN_NS);
	t1 = time_from_ns(3);

	CU_ASSERT(odp_time_cmp(t2, t1) > 0);
	CU_ASSERT(odp_time_cmp(t1, t2) < 0);

	t1 = time_from_ns(0);
	CU_ASSERT(odp_time_cmp(t1, ODP_TIME_NULL) == 0);
}

static void time_test_local_cmp(void)
{
	time_test_cmp(odp_time_local, odp_time_local_from_ns);
}

static void time_test_global_cmp(void)
{
	time_test_cmp(odp_time_global, odp_time_global_from_ns);
}

static void time_test_local_strict_cmp(void)
{
	time_test_cmp(odp_time_local_strict, odp_time_local_from_ns);
}

static void time_test_global_strict_cmp(void)
{
	time_test_cmp(odp_time_global_strict, odp_time_global_from_ns);
}

/* check that a time difference gives a reasonable result */
static void time_test_diff(time_cb time_cur,
			   time_from_ns_cb time_from_ns,
			   uint64_t res)
{
	/* volatile to stop optimization of busy loop */
	volatile int count = 0;
	odp_time_t diff, t1, t2;
	uint64_t ns1, ns2, ns;
	uint64_t nsdiff, diff_ns;
	uint64_t upper_limit, lower_limit;

	/* test timestamp diff */
	t1 = time_cur();

	while (count < BUSY_LOOP_CNT) {
		count++;
	};

	t2 = time_cur();
	CU_ASSERT(odp_time_cmp(t2, t1) > 0);

	diff = odp_time_diff(t2, t1);
	CU_ASSERT(odp_time_cmp(diff, ODP_TIME_NULL) > 0);

	diff_ns = odp_time_diff_ns(t2, t1);
	CU_ASSERT(diff_ns > 0);

	ns1 = odp_time_to_ns(t1);
	ns2 = odp_time_to_ns(t2);
	ns = ns2 - ns1;
	nsdiff = odp_time_to_ns(diff);

	upper_limit = ns + 2 * res;
	lower_limit = ns - 2 * res;
	CU_ASSERT((nsdiff <= upper_limit) && (nsdiff >= lower_limit));
	CU_ASSERT((diff_ns <= upper_limit) && (diff_ns >= lower_limit));

	/* test timestamp and interval diff */
	ns1 = 54;
	t1 = time_from_ns(ns1);
	ns = ns2 - ns1;

	diff = odp_time_diff(t2, t1);
	CU_ASSERT(odp_time_cmp(diff, ODP_TIME_NULL) > 0);

	diff_ns = odp_time_diff_ns(t2, t1);
	CU_ASSERT(diff_ns > 0);

	nsdiff = odp_time_to_ns(diff);

	upper_limit = ns + 2 * res;
	lower_limit = ns - 2 * res;
	CU_ASSERT((nsdiff <= upper_limit) && (nsdiff >= lower_limit));
	CU_ASSERT((diff_ns <= upper_limit) && (diff_ns >= lower_limit));

	/* test interval diff */
	ns2 = 60 * 10 * ODP_TIME_SEC_IN_NS;
	ns = ns2 - ns1;

	t2 = time_from_ns(ns2);
	diff = odp_time_diff(t2, t1);
	CU_ASSERT(odp_time_cmp(diff, ODP_TIME_NULL) > 0);

	diff_ns = odp_time_diff_ns(t2, t1);
	CU_ASSERT(diff_ns > 0);

	nsdiff = odp_time_to_ns(diff);

	upper_limit = ns + 2 * res;
	lower_limit = ns - 2 * res;
	CU_ASSERT((nsdiff <= upper_limit) && (nsdiff >= lower_limit));
	CU_ASSERT((diff_ns <= upper_limit) && (diff_ns >= lower_limit));

	/* same time has to diff to 0 */
	diff = odp_time_diff(t2, t2);
	CU_ASSERT(odp_time_cmp(diff, ODP_TIME_NULL) == 0);

	diff = odp_time_diff(t2, ODP_TIME_NULL);
	CU_ASSERT(odp_time_cmp(t2, diff) == 0);

	diff_ns = odp_time_diff_ns(t2, t2);
	CU_ASSERT(diff_ns == 0);
}

static void time_test_local_diff(void)
{
	time_test_diff(odp_time_local, odp_time_local_from_ns, local_res);
}

static void time_test_global_diff(void)
{
	time_test_diff(odp_time_global, odp_time_global_from_ns, global_res);
}

static void time_test_local_strict_diff(void)
{
	time_test_diff(odp_time_local_strict, odp_time_local_from_ns, local_res);
}

static void time_test_global_strict_diff(void)
{
	time_test_diff(odp_time_global_strict, odp_time_global_from_ns, global_res);
}

/* check that a time sum gives a reasonable result */
static void time_test_sum(time_cb time_cur,
			  time_from_ns_cb time_from_ns,
			  uint64_t res)
{
	odp_time_t sum, t1, t2;
	uint64_t nssum, ns1, ns2, ns;
	uint64_t upper_limit, lower_limit;

	/* sum timestamp and interval */
	t1 = time_cur();
	ns2 = 103;
	t2 = time_from_ns(ns2);
	ns1 = odp_time_to_ns(t1);
	ns = ns1 + ns2;

	sum = odp_time_sum(t2, t1);
	CU_ASSERT(odp_time_cmp(sum, ODP_TIME_NULL) > 0);
	nssum = odp_time_to_ns(sum);

	upper_limit = ns + 2 * res;
	lower_limit = ns - 2 * res;
	CU_ASSERT((nssum <= upper_limit) && (nssum >= lower_limit));

	/* sum intervals */
	ns1 = 60 * 13 * ODP_TIME_SEC_IN_NS;
	t1 = time_from_ns(ns1);
	ns = ns1 + ns2;

	sum = odp_time_sum(t2, t1);
	CU_ASSERT(odp_time_cmp(sum, ODP_TIME_NULL) > 0);
	nssum = odp_time_to_ns(sum);

	upper_limit = ns + 2 * res;
	lower_limit = ns - 2 * res;
	CU_ASSERT((nssum <= upper_limit) && (nssum >= lower_limit));

	/* test on 0 */
	sum = odp_time_sum(t2, ODP_TIME_NULL);
	CU_ASSERT(odp_time_cmp(t2, sum) == 0);
}

static void time_test_local_sum(void)
{
	time_test_sum(odp_time_local, odp_time_local_from_ns, local_res);
}

static void time_test_global_sum(void)
{
	time_test_sum(odp_time_global, odp_time_global_from_ns, global_res);
}

static void time_test_local_strict_sum(void)
{
	time_test_sum(odp_time_local_strict, odp_time_local_from_ns, local_res);
}

static void time_test_global_strict_sum(void)
{
	time_test_sum(odp_time_global_strict, odp_time_global_from_ns, global_res);
}

static void time_test_wait_until(time_cb time_cur, time_from_ns_cb time_from_ns)
{
	int i;
	odp_time_t lower_limit, upper_limit;
	odp_time_t start_time, end_time, wait;
	odp_time_t second = time_from_ns(ODP_TIME_SEC_IN_NS);

	start_time = time_cur();
	wait = start_time;
	for (i = 0; i < WAIT_SECONDS; i++) {
		wait = odp_time_sum(wait, second);
		odp_time_wait_until(wait);
	}
	end_time = time_cur();

	wait = odp_time_diff(end_time, start_time);
	lower_limit = time_from_ns(WAIT_SECONDS * ODP_TIME_SEC_IN_NS -
				   DELAY_TOLERANCE);
	upper_limit = time_from_ns(WAIT_SECONDS * ODP_TIME_SEC_IN_NS +
				   DELAY_TOLERANCE);

	if (odp_time_cmp(wait, lower_limit) < 0) {
		fprintf(stderr, "Exceed lower limit: "
			"wait is %" PRIu64 ", lower_limit %" PRIu64 "\n",
			odp_time_to_ns(wait), odp_time_to_ns(lower_limit));
		CU_FAIL("Exceed lower limit\n");
	}

	if (odp_time_cmp(wait, upper_limit) > 0) {
		fprintf(stderr, "Exceed upper limit: "
			"wait is %" PRIu64 ", upper_limit %" PRIu64 "\n",
			odp_time_to_ns(wait), odp_time_to_ns(lower_limit));
		CU_FAIL("Exceed upper limit\n");
	}
}

static void time_test_local_wait_until(void)
{
	time_test_wait_until(odp_time_local, odp_time_local_from_ns);
}

static void time_test_global_wait_until(void)
{
	time_test_wait_until(odp_time_global, odp_time_global_from_ns);
}

static void time_test_wait_ns(void)
{
	int i;
	odp_time_t lower_limit, upper_limit;
	odp_time_t start_time, end_time, diff;

	start_time = odp_time_local();
	for (i = 0; i < WAIT_SECONDS; i++)
		odp_time_wait_ns(ODP_TIME_SEC_IN_NS);
	end_time = odp_time_local();

	diff = odp_time_diff(end_time, start_time);

	lower_limit = odp_time_local_from_ns(WAIT_SECONDS * ODP_TIME_SEC_IN_NS -
					     DELAY_TOLERANCE);
	upper_limit = odp_time_local_from_ns(WAIT_SECONDS * ODP_TIME_SEC_IN_NS +
					     DELAY_TOLERANCE);

	if (odp_time_cmp(diff, lower_limit) < 0) {
		fprintf(stderr, "Exceed lower limit: "
			"diff is %" PRIu64 ", lower_limit %" PRIu64 "\n",
			odp_time_to_ns(diff), odp_time_to_ns(lower_limit));
		CU_FAIL("Exceed lower limit\n");
	}

	if (odp_time_cmp(diff, upper_limit) > 0) {
		fprintf(stderr, "Exceed upper limit: "
			"diff is %" PRIu64 ", upper_limit %" PRIu64 "\n",
			odp_time_to_ns(diff), odp_time_to_ns(upper_limit));
		CU_FAIL("Exceed upper limit\n");
	}
}

/* Check that ODP time is within +-5% of system time */
static void check_time_diff(double t_odp, double t_system,
			    const char *test, int id)
{
	if (t_odp > t_system * 1.05) {
		CU_FAIL("ODP time too high");
		fprintf(stderr, "ODP time too high (%s/%d): t_odp: %f, t_system: %f\n",
			test, id, t_odp, t_system);
	}
	if (t_odp < t_system * 0.95) {
		CU_FAIL("ODP time too low");
		fprintf(stderr, "ODP time too low (%s/%d): t_odp: %f, t_system: %f\n",
			test, id, t_odp, t_system);
	}
}

static void time_test_accuracy(time_cb time_cur,
			       time_cb time_cur_strict, time_from_ns_cb time_from_ns)
{
	int i;
	odp_time_t t1[2], t2[2], wait;
	struct timespec ts1, ts2, tsdiff;
	double sec_c;
	odp_time_t sec = time_from_ns(ODP_TIME_SEC_IN_NS);

	i = clock_gettime(CLOCK_MONOTONIC, &ts1);
	CU_ASSERT(i == 0);
	t1[0] = time_cur_strict();
	t1[1] = time_cur();

	wait = odp_time_sum(t1[0], sec);
	for (i = 0; i < 5; i++) {
		odp_time_wait_until(wait);
		wait = odp_time_sum(wait, sec);
	}

	i = clock_gettime(CLOCK_MONOTONIC, &ts2);
	CU_ASSERT(i == 0);
	t2[0] = time_cur_strict();
	t2[1] = time_cur();

	if (ts2.tv_nsec < ts1.tv_nsec) {
		tsdiff.tv_nsec = 1000000000L + ts2.tv_nsec - ts1.tv_nsec;
		tsdiff.tv_sec = ts2.tv_sec - 1 - ts1.tv_sec;
	} else {
		tsdiff.tv_nsec = ts2.tv_nsec - ts1.tv_nsec;
		tsdiff.tv_sec = ts2.tv_sec - ts1.tv_sec;
	}
	sec_c = ((double)(tsdiff.tv_nsec) / 1000000000L) + tsdiff.tv_sec;

	for (i = 0; i < 2; i++) {
		odp_time_t diff  = odp_time_diff(t2[i], t1[i]);
		double sec_t = ((double)odp_time_to_ns(diff)) / ODP_TIME_SEC_IN_NS;

		check_time_diff(sec_t, sec_c, __func__, i);
	}
}

static void time_test_local_accuracy(void)
{
	time_test_accuracy(odp_time_local, odp_time_local_strict, odp_time_local_from_ns);
}

static void time_test_global_accuracy(void)
{
	time_test_accuracy(odp_time_global, odp_time_global_strict, odp_time_global_from_ns);
}

static void time_test_accuracy_nsec(void)
{
	uint64_t t1[4], t2[4];
	struct timespec ts1, ts2, tsdiff;
	double sec_c;
	int i, ret;

	ret = clock_gettime(CLOCK_MONOTONIC, &ts1);
	CU_ASSERT(ret == 0);
	t1[0] = odp_time_global_strict_ns();
	t1[1] = odp_time_local_strict_ns();
	t1[2] = odp_time_global_ns();
	t1[3] = odp_time_local_ns();

	for (i = 0; i < 5; i++)
		odp_time_wait_ns(ODP_TIME_SEC_IN_NS);

	ret = clock_gettime(CLOCK_MONOTONIC, &ts2);
	CU_ASSERT(ret == 0);
	t2[0] = odp_time_global_strict_ns();
	t2[1] = odp_time_local_strict_ns();
	t2[2] = odp_time_global_ns();
	t2[3] = odp_time_local_ns();

	if (ts2.tv_nsec < ts1.tv_nsec) {
		tsdiff.tv_nsec = 1000000000L + ts2.tv_nsec - ts1.tv_nsec;
		tsdiff.tv_sec = ts2.tv_sec - 1 - ts1.tv_sec;
	} else {
		tsdiff.tv_nsec = ts2.tv_nsec - ts1.tv_nsec;
		tsdiff.tv_sec = ts2.tv_sec - ts1.tv_sec;
	}
	sec_c = ((double)(tsdiff.tv_nsec) / 1000000000L) + tsdiff.tv_sec;

	for (i = 0; i < 4; i++) {
		uint64_t diff  = t2[i] - t1[i];
		double sec_t = ((double)diff) / ODP_TIME_SEC_IN_NS;

		check_time_diff(sec_t, sec_c, __func__, i);
	}
}

odp_testinfo_t time_suite_time[] = {
	ODP_TEST_INFO(time_test_constants),
	ODP_TEST_INFO(time_test_local_res),
	ODP_TEST_INFO(time_test_local_conversion),
	ODP_TEST_INFO(time_test_local_cmp),
	ODP_TEST_INFO(time_test_local_diff),
	ODP_TEST_INFO(time_test_local_sum),
	ODP_TEST_INFO(time_test_global_res),
	ODP_TEST_INFO(time_test_global_conversion),
	ODP_TEST_INFO(time_test_global_cmp),
	ODP_TEST_INFO(time_test_global_diff),
	ODP_TEST_INFO(time_test_global_sum),
	ODP_TEST_INFO(time_test_wait_ns),
	ODP_TEST_INFO(time_test_monotony),
	ODP_TEST_INFO(time_test_local_wait_until),
	ODP_TEST_INFO(time_test_global_wait_until),
	ODP_TEST_INFO(time_test_local_accuracy),
	ODP_TEST_INFO(time_test_global_accuracy),
	ODP_TEST_INFO(time_test_accuracy_nsec),
	ODP_TEST_INFO(time_test_local_strict_diff),
	ODP_TEST_INFO(time_test_local_strict_sum),
	ODP_TEST_INFO(time_test_local_strict_cmp),
	ODP_TEST_INFO(time_test_global_strict_diff),
	ODP_TEST_INFO(time_test_global_strict_sum),
	ODP_TEST_INFO(time_test_global_strict_cmp),
	ODP_TEST_INFO_NULL
};

odp_suiteinfo_t time_suites[] = {
		{"Time", NULL, NULL, time_suite_time},
		ODP_SUITE_INFO_NULL
};

int main(int argc, char *argv[])
{
	int ret;

	/* parse common options: */
	if (odp_cunit_parse_options(argc, argv))
		return -1;

	ret = odp_cunit_register(time_suites);

	if (ret == 0)
		ret = odp_cunit_run();

	return ret;
}
