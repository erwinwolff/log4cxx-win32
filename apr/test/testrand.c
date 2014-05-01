/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_general.h"
#include "apr_pools.h"
#include "apr_random.h"
#include "apr_thread_proc.h"
#include <stdio.h>
#include <stdlib.h>
#include "testutil.h"

#define RANDOM_BUF_SZ 128

static void hexdump(const char *msg, const unsigned char *b, int n)
{
    int i;

    printf("\n%s", msg);
    for (i = 0; i < n; ++i) {
#if 0
        if ((i & 0xf) == 0)
            printf("%04x", i);
        printf(" %02x", b[i]);
        if ((i & 0xf) == 0xf)
            printf("\n");
#else
        printf("0x%02x,", b[i]);
        if ((i & 7) == 7)
            printf("\n");
#endif
    }
    printf("\n");
}

static apr_random_t *r;

typedef apr_status_t APR_THREAD_FUNC rnd_fn(apr_random_t * r, void *b,
                                            apr_size_t n);

static void rand_run_kat(abts_case *tc, rnd_fn *f, apr_random_t *r,
                         const unsigned char expected[RANDOM_BUF_SZ])
{
    unsigned char c[RANDOM_BUF_SZ];
    apr_status_t rv;

    rv = f(r, c, RANDOM_BUF_SZ);
    ABTS_INT_EQUAL(tc, 0, rv);
    if (rv)
        return;
    if (memcmp(c, expected, RANDOM_BUF_SZ)) {
        hexdump("Generated: ", c, RANDOM_BUF_SZ);
        hexdump("Expected:  ", expected, RANDOM_BUF_SZ);
        ABTS_FAIL(tc, "Randomness mismatch");
    }
}

#if APR_HAS_FORK
static int rand_check_kat(rnd_fn *f, apr_random_t *r,
                          const unsigned char expected[RANDOM_BUF_SZ],
                          apr_file_t *readp, apr_file_t *writep)
{
    apr_size_t nbytes = RANDOM_BUF_SZ;
    apr_size_t cmd_size = 1;
    unsigned char c[RANDOM_BUF_SZ];
    char ack;
    apr_status_t rv;

    rv = f(r, c, RANDOM_BUF_SZ);
    if (rv)
        return 2;
    rv = 0;
    if (memcmp(c, expected, RANDOM_BUF_SZ)) {
        rv = 1;
    } else {
        hexdump("Generated: ", c, RANDOM_BUF_SZ);
        hexdump("Previous:  ", expected, RANDOM_BUF_SZ);
    }
    /* Report back our random values for comparison in another child */
    apr_file_write(writep, c, &nbytes);
    /* Wait for our parent ack the data */
    apr_file_read(readp, &ack, &cmd_size);
    return rv;
}
#endif

static void rand_add_zeroes(apr_random_t *r)
{
    static unsigned char c[2048];

    apr_random_add_entropy(r, c, sizeof c);
}

static void rand_run_seed_short(abts_case *tc, rnd_fn *f, apr_random_t *r,
                                int count)
{
    int i;
    apr_status_t rv;
    char c[1];

    for (i = 0; i < count; ++i)
        rand_add_zeroes(r);
    rv = f(r, c, 1);
    ABTS_INT_EQUAL(tc, 1, APR_STATUS_IS_ENOTENOUGHENTROPY(rv));
}

static void rand_seed_short(abts_case *tc, void *data)
{
    r = apr_random_standard_new(p);
    rand_run_seed_short(tc, apr_random_insecure_bytes, r, 32);
}

static void rand_kat(abts_case *tc, void *data)
{
    unsigned char expected[RANDOM_BUF_SZ] = {
        0x82, 0x04, 0xad, 0xd2, 0x0b, 0xd5, 0xac, 0xda,
        0x3d, 0x85, 0x58, 0x38, 0x54, 0x6b, 0x69, 0x45,
        0x37, 0x4c, 0xc7, 0xd7, 0x87, 0xeb, 0xbf, 0xd9,
        0xb1, 0xb8, 0xb8, 0x2d, 0x9b, 0x33, 0x6e, 0x97,
        0x04, 0x1d, 0x4c, 0xb0, 0xd1, 0xdf, 0x3d, 0xac,
        0xd2, 0xaa, 0xfa, 0xcd, 0x96, 0xb7, 0xcf, 0xb1,
        0x8e, 0x3d, 0xb3, 0xe5, 0x37, 0xa9, 0x95, 0xb4,
        0xaa, 0x3d, 0x11, 0x1a, 0x08, 0x20, 0x21, 0x9f,
        0xdb, 0x08, 0x3a, 0xb9, 0x57, 0x9f, 0xf2, 0x1f,
        0x27, 0xdc, 0xb6, 0xc0, 0x85, 0x08, 0x05, 0xbb,
        0x13, 0xbe, 0xb1, 0xe9, 0x63, 0x2a, 0xe2, 0xa4,
        0x23, 0x15, 0x2a, 0x10, 0xbf, 0xdf, 0x09, 0xb3,
        0xc7, 0xfb, 0x2d, 0x87, 0x48, 0x19, 0xfb, 0xc0,
        0x15, 0x8c, 0xcb, 0xc6, 0xbd, 0x89, 0x38, 0x69,
        0xa3, 0xae, 0xa3, 0x21, 0x58, 0x50, 0xe7, 0xc4,
        0x87, 0xec, 0x2e, 0xb1, 0x2d, 0x6a, 0xbd, 0x46
    };

    rand_add_zeroes(r);
    rand_run_kat(tc, apr_random_insecure_bytes, r, expected);
}

static void rand_seed_short2(abts_case *tc, void *data)
{
    rand_run_seed_short(tc, apr_random_secure_bytes, r, 320);
}

static void rand_kat2(abts_case *tc, void *data)
{
    unsigned char expected[RANDOM_BUF_SZ] = {
        0x38, 0x8f, 0x01, 0x29, 0x5a, 0x5c, 0x1f, 0xa8,
        0x00, 0xde, 0x16, 0x4c, 0xe5, 0xf7, 0x1f, 0x58,
        0xc0, 0x67, 0xe2, 0x98, 0x3d, 0xde, 0x4a, 0x75,
        0x61, 0x3f, 0x23, 0xd8, 0x45, 0x7a, 0x10, 0x60,
        0x59, 0x9b, 0xd6, 0xaf, 0xcb, 0x0a, 0x2e, 0x34,
        0x9c, 0x39, 0x5b, 0xd0, 0xbc, 0x9a, 0xf0, 0x7b,
        0x7f, 0x40, 0x8b, 0x33, 0xc0, 0x0e, 0x2a, 0x56,
        0xfc, 0xe5, 0xab, 0xde, 0x7b, 0x13, 0xf5, 0xec,
        0x15, 0x68, 0xb8, 0x09, 0xbc, 0x2c, 0x15, 0xf0,
        0x7b, 0xef, 0x2a, 0x97, 0x19, 0xa8, 0x69, 0x51,
        0xdf, 0xb0, 0x5f, 0x1a, 0x4e, 0xdf, 0x42, 0x02,
        0x71, 0x36, 0xa7, 0x25, 0x64, 0x85, 0xe2, 0x72,
        0xc7, 0x87, 0x4d, 0x7d, 0x15, 0xbb, 0x15, 0xd1,
        0xb1, 0x62, 0x0b, 0x25, 0xd9, 0xd3, 0xd9, 0x5a,
        0xe3, 0x47, 0x1e, 0xae, 0x67, 0xb4, 0x19, 0x9e,
        0xed, 0xd2, 0xde, 0xce, 0x18, 0x70, 0x57, 0x12
    };

    rand_add_zeroes(r);
    rand_run_kat(tc, apr_random_secure_bytes, r, expected);
}

static void rand_barrier(abts_case *tc, void *data)
{
    apr_random_barrier(r);
    rand_run_seed_short(tc, apr_random_secure_bytes, r, 320);
}

static void rand_kat3(abts_case *tc, void *data)
{
    unsigned char expected[RANDOM_BUF_SZ] = {
        0xe8, 0xe7, 0xc9, 0x45, 0xe2, 0x2a, 0x54, 0xb2,
        0xdd, 0xe0, 0xf9, 0xbc, 0x3d, 0xf9, 0xce, 0x3c,
        0x4c, 0xbd, 0xc9, 0xe2, 0x20, 0x4a, 0x35, 0x1c,
        0x04, 0x52, 0x7f, 0xb8, 0x0f, 0x60, 0x89, 0x63,
        0x8a, 0xbe, 0x0a, 0x44, 0xac, 0x5d, 0xd8, 0xeb,
        0x24, 0x7d, 0xd1, 0xda, 0x4d, 0x86, 0x9b, 0x94,
        0x26, 0x56, 0x4a, 0x5e, 0x30, 0xea, 0xd4, 0xa9,
        0x9a, 0xdf, 0xdd, 0xb6, 0xb1, 0x15, 0xe0, 0xfa,
        0x28, 0xa4, 0xd6, 0x95, 0xa4, 0xf1, 0xd8, 0x6e,
        0xeb, 0x8c, 0xa4, 0xac, 0x34, 0xfe, 0x06, 0x92,
        0xc5, 0x09, 0x99, 0x86, 0xdc, 0x5a, 0x3c, 0x92,
        0xc8, 0x3e, 0x52, 0x00, 0x4d, 0x01, 0x43, 0x6f,
        0x69, 0xcf, 0xe2, 0x60, 0x9c, 0x23, 0xb3, 0xa5,
        0x5f, 0x51, 0x47, 0x8c, 0x07, 0xde, 0x60, 0xc6,
        0x04, 0xbf, 0x32, 0xd6, 0xdc, 0xb7, 0x31, 0x01,
        0x29, 0x51, 0x51, 0xb3, 0x19, 0x6e, 0xe4, 0xf8
    };

    rand_run_kat(tc, apr_random_insecure_bytes, r, expected);
}

static void rand_kat4(abts_case *tc, void *data)
{
    unsigned char expected[RANDOM_BUF_SZ] = {
        0x7d, 0x0e, 0xc4, 0x4e, 0x3e, 0xac, 0x86, 0x50,
        0x37, 0x95, 0x7a, 0x98, 0x23, 0x26, 0xa7, 0xbf,
        0x60, 0xfb, 0xa3, 0x70, 0x90, 0xc3, 0x58, 0xc6,
        0xbd, 0xd9, 0x5e, 0xa6, 0x77, 0x62, 0x7a, 0x5c,
        0x96, 0x83, 0x7f, 0x80, 0x3d, 0xf4, 0x9c, 0xcc,
        0x9b, 0x0c, 0x8c, 0xe1, 0x72, 0xa8, 0xfb, 0xc9,
        0xc5, 0x43, 0x91, 0xdc, 0x9d, 0x92, 0xc2, 0xce,
        0x1c, 0x5e, 0x36, 0xc7, 0x87, 0xb1, 0xb4, 0xa3,
        0xc8, 0x69, 0x76, 0xfc, 0x35, 0x75, 0xcb, 0x08,
        0x2f, 0xe3, 0x98, 0x76, 0x37, 0x80, 0x04, 0x5c,
        0xb8, 0xb0, 0x7f, 0xb2, 0xda, 0xe3, 0xa3, 0xba,
        0xed, 0xff, 0xf5, 0x9d, 0x3b, 0x7b, 0xf3, 0x32,
        0x6c, 0x50, 0xa5, 0x3e, 0xcc, 0xe1, 0x84, 0x9c,
        0x17, 0x9e, 0x80, 0x64, 0x09, 0xbb, 0x62, 0xf1,
        0x95, 0xf5, 0x2c, 0xc6, 0x9f, 0x6a, 0xee, 0x6d,
        0x17, 0x35, 0x5f, 0x35, 0x8d, 0x55, 0x0c, 0x07
    };

    rand_add_zeroes(r);
    rand_run_kat(tc, apr_random_secure_bytes, r, expected);
}

#if APR_HAS_FORK
static void rand_fork(abts_case *tc, void *data)
{
    apr_proc_t proc;
    apr_status_t rv;
    apr_size_t nbytes = RANDOM_BUF_SZ;
    apr_size_t cmd_size = 1;
    char cmd = 'X';
    unsigned char expected[RANDOM_BUF_SZ] = {
        0xac, 0x93, 0xd2, 0x5c, 0xc7, 0xf5, 0x8d, 0xc2,
        0xd8, 0x8d, 0xb6, 0x7a, 0x94, 0xe1, 0x83, 0x4c,
        0x26, 0xe2, 0x38, 0x6d, 0xf5, 0xbd, 0x9d, 0x6e,
        0x91, 0x77, 0x3a, 0x4b, 0x9b, 0xef, 0x9b, 0xa3,
        0x9f, 0xf6, 0x6d, 0x0c, 0xdc, 0x4b, 0x02, 0xe9,
        0x5d, 0x3d, 0xfc, 0x92, 0x6b, 0xdf, 0xc9, 0xef,
        0xb9, 0xa8, 0x74, 0x09, 0xa3, 0xff, 0x64, 0x8d,
        0x19, 0xc1, 0x31, 0x31, 0x17, 0xe1, 0xb7, 0x7a,
        0xe7, 0x55, 0x14, 0x92, 0x05, 0xe3, 0x1e, 0xb8,
        0x9b, 0x1b, 0xdc, 0xac, 0x0e, 0x15, 0x08, 0xa2,
        0x93, 0x13, 0xf6, 0x04, 0xc6, 0x9d, 0xf8, 0x7f,
        0x26, 0x32, 0x68, 0x43, 0x2e, 0x5a, 0x4f, 0x47,
        0xe8, 0xf8, 0x59, 0xb7, 0xfb, 0xbe, 0x30, 0x04,
        0xb6, 0x63, 0x6f, 0x19, 0xf3, 0x2c, 0xd4, 0xeb,
        0x32, 0x8a, 0x54, 0x01, 0xd0, 0xaf, 0x3f, 0x13,
        0xc1, 0x7f, 0x10, 0x2e, 0x08, 0x1c, 0x28, 0x4b,
    };

    apr_file_t *readdatap = NULL;
    apr_file_t *writedatap = NULL;
    apr_file_t *readcmdp = NULL;
    apr_file_t *writecmdp = NULL;
    apr_pool_t *p;
    int i;

    apr_pool_create(&p, NULL);
    /* Set up data pipe for children */
    rv = apr_file_pipe_create(&readdatap, &writedatap, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_NOTNULL(tc, readdatap);
    ABTS_PTR_NOTNULL(tc, writedatap);
    /* Set up cmd pipe for children */
    rv = apr_file_pipe_create(&readcmdp, &writecmdp, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_NOTNULL(tc, readcmdp);
    ABTS_PTR_NOTNULL(tc, writecmdp);

    rand_run_kat(tc, apr_random_secure_bytes, r, expected);

    for (i = 0; i< 10; i++)
    {
        rv = apr_proc_fork(&proc, p);
        if (rv == APR_INCHILD) {
            int n = rand_check_kat(apr_random_secure_bytes, r, expected, readcmdp, writedatap);
            exit(n);
        }
        else if (rv == APR_INPARENT) {
            int exitcode;
            apr_exit_why_e why;

            /* Read the random data generated by child */
            rv = apr_file_read(readdatap, expected, &nbytes);
            ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
            /* Tell child to finish */
            rv = apr_file_write(writecmdp, &cmd, &cmd_size);
            ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
            apr_proc_wait(&proc, &exitcode, &why, APR_WAIT);
            if (why != APR_PROC_EXIT) {
                ABTS_FAIL(tc, "Child terminated abnormally");
            }
            else if (exitcode == 0) {
                if (i == 0)
                {
                    ABTS_FAIL(tc, "Child produced our randomness");
                } else
                {
                    ABTS_FAIL(tc, "Child produced randomness of previous child");
                }
            }
            else if (exitcode == 2) {
                ABTS_FAIL(tc, "Child randomness failed");
            }
            else if (exitcode != 1) {
                ABTS_FAIL(tc, "Unknown child error");
            }
        } else {
            ABTS_FAIL(tc, "Fork failed");
        }
    }

}
#endif

static void rand_exists(abts_case *tc, void *data)
{
#if !APR_HAS_RANDOM
    ABTS_NOT_IMPL(tc, "apr_generate_random_bytes");
#else
    unsigned char c[42];

    /* There must be a better way to test random-ness, but I don't know
     * what it is right now.
     */
    APR_ASSERT_SUCCESS(tc, "apr_generate_random_bytes failed",
                       apr_generate_random_bytes(c, sizeof c));
#endif
}

abts_suite *testrand(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, rand_exists, NULL);
    abts_run_test(suite, rand_seed_short, NULL);
    abts_run_test(suite, rand_kat, NULL);
    abts_run_test(suite, rand_seed_short2, NULL);
    abts_run_test(suite, rand_kat2, NULL);
    abts_run_test(suite, rand_barrier, NULL);
    abts_run_test(suite, rand_kat3, NULL);
    abts_run_test(suite, rand_kat4, NULL);
#if APR_HAS_FORK
    abts_run_test(suite, rand_fork, NULL);
#endif

    return suite;
}
