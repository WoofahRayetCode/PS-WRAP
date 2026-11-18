// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <munit.h>

#include <chiaki/seqnum.h>
#include <chiaki/packetstats.h>


static MunitResult test_seq_num_16(const MunitParameter params[], void *user)
{
	ChiakiSeqNum16 a = 0;
	do
	{
		ChiakiSeqNum16 b = a + 1;
		munit_assert(chiaki_seq_num_16_gt(b, a));
		munit_assert(!chiaki_seq_num_16_gt(a, b));
		munit_assert(chiaki_seq_num_16_lt(a, b));
		munit_assert(!chiaki_seq_num_16_lt(b, a));
		a = b;
	} while(a);

	a = 0;
	do
	{
		ChiakiSeqNum16 b = a + 0xfff;
		munit_assert(chiaki_seq_num_16_gt(b, a));
		munit_assert(!chiaki_seq_num_16_gt(a, b));
		munit_assert(chiaki_seq_num_16_lt(a, b));
		munit_assert(!chiaki_seq_num_16_lt(b, a));
		a++;
	} while(a);

	munit_assert(chiaki_seq_num_16_gt(1, 0xfff5));
	munit_assert(!chiaki_seq_num_16_gt(0xfff5, 1));

	return MUNIT_OK;
}


static MunitResult test_seq_num_32(const MunitParameter params[], void *user)
{
	munit_assert(chiaki_seq_num_32_gt(1, 0));
	munit_assert(!chiaki_seq_num_32_gt(0, 1));
	munit_assert(!chiaki_seq_num_32_lt(1, 0));
	munit_assert(chiaki_seq_num_32_lt(0, 1));
	munit_assert(chiaki_seq_num_32_gt(1, 0xfffffff5));
	munit_assert(!chiaki_seq_num_32_gt(0xfffffff5, 1));

	return MUNIT_OK;
}



static MunitResult test_packet_stats_rollover(const MunitParameter params[], void *user)
{
	ChiakiPacketStats stats;
	uint64_t received, lost;
	munit_assert_int(chiaki_packet_stats_init(&stats), ==, CHIAKI_ERR_SUCCESS);

	// Starting near the rollover must not create a gap back to sequence zero.
	chiaki_packet_stats_push_seq(&stats, 0xfffe);
	chiaki_packet_stats_get(&stats, true, &received, &lost);
	munit_assert_uint64(received, ==, 1);
	munit_assert_uint64(lost, ==, 0);

	chiaki_packet_stats_push_seq(&stats, 0xffff);
	chiaki_packet_stats_push_seq(&stats, 1);
	chiaki_packet_stats_push_seq(&stats, 0xfffe); // Late duplicate from the previous report.
	chiaki_packet_stats_push_generation(&stats, 10, 2);
	chiaki_packet_stats_get(&stats, false, &received, &lost);
	munit_assert_uint64(received, ==, 12);
	munit_assert_uint64(lost, ==, 3);

	chiaki_packet_stats_push_seq(&stats, 0); // Fills the gap before the report is reset.
	chiaki_packet_stats_push_seq(&stats, 0); // Must never turn an extra packet into lost packets.
	chiaki_packet_stats_get(&stats, true, &received, &lost);
	munit_assert_uint64(lost, ==, 2);

	chiaki_packet_stats_push_seq(&stats, 2);
	chiaki_packet_stats_get(&stats, true, &received, &lost);
	munit_assert_uint64(received, ==, 1);
	munit_assert_uint64(lost, ==, 0);
	chiaki_packet_stats_fini(&stats);
	return MUNIT_OK;
}

MunitTest tests_seq_num[] = {
	{
		"/seq_num_16",
		test_seq_num_16,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},
	{
		"/seq_num_32",
		test_seq_num_32,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},
	{
		"/packet_stats_rollover",
		test_packet_stats_rollover,
		NULL,
		NULL,
		MUNIT_TEST_OPTION_NONE,
		NULL
	},
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
