/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2014 by Delphix. All rights reserved.
 */

#ifndef _VDEV_RAIDZ_H
#define _VDEV_RAIDZ_H

#define VDEV_RAIDZ_P        0
#define VDEV_RAIDZ_Q        1
#define VDEV_RAIDZ_R        2

#define	VDEV_RAIDZ_MUL_2(x)	(((x) << 1) ^ (((x) & 0x80) ? 0x1d : 0))
#define	VDEV_RAIDZ_MUL_4(x)	(VDEV_RAIDZ_MUL_2(VDEV_RAIDZ_MUL_2(x)))

/*
 * We provide a mechanism to perform the field multiplication operation on a
 * 64-bit value all at once rather than a byte at a time. This works by
 * creating a mask from the top bit in each byte and using that to
 * conditionally apply the XOR of 0x1d.
 */
#define VDEV_RAIDZ_64MUL_2(x, mask) \
{ \
        (mask) = (x) & 0x8080808080808080ULL; \
        (mask) = ((mask) << 1) - ((mask) >> 7); \
        (x) = (((x) << 1) & 0xfefefefefefefefeULL) ^ \
            ((mask) & 0x1d1d1d1d1d1d1d1dULL); \
}

#define VDEV_RAIDZ_64MUL_4(x, mask) \
{ \
        VDEV_RAIDZ_64MUL_2((x), mask); \
        VDEV_RAIDZ_64MUL_2((x), mask); \
}

#if defined(_KERNEL ) && defined(__x86_64__)
#include <asm/i387.h>
#endif

#if defined(_KERNEL)
#define kfpu_begin() kernel_fpu_begin()
#define kfpu_end() kernel_fpu_end()
#else
#define kfpu_begin() ((void)0)
#define kfpu_end() ((void)0)
#endif

//Vectorized Modes
#define VDEV_RAIDZ_VECTORIZED_OFF 0
#define VDEV_RAIDZ_VECTORIZED_SSE4 1
#define VDEV_RAIDZ_VECTORIZED_AVX 2

typedef struct raidz_col {
    uint64_t rc_devidx;     /* child device index for I/O */
    uint64_t rc_offset;     /* device offset */
    uint64_t rc_size;       /* I/O size */
    void *rc_data;          /* I/O data */
    void *rc_gdata;         /* used to store the "good" version */
    int rc_error;           /* I/O error for this device */
    uint8_t rc_tried;       /* Did we attempt this I/O column? */
    uint8_t rc_skipped;     /* Did we skip this I/O column? */
} raidz_col_t;

typedef struct raidz_map {
    uint64_t rm_cols;       /* Regular column count */
    uint64_t rm_scols;      /* Count including skipped columns */
    uint64_t rm_bigcols;        /* Number of oversized columns */
    uint64_t rm_asize;      /* Actual total I/O size */
    uint64_t rm_missingdata;    /* Count of missing data devices */
    uint64_t rm_missingparity;  /* Count of missing parity devices */
    uint64_t rm_firstdatacol;   /* First data column/parity count */
    uint64_t rm_nskip;      /* Skipped sectors for padding */
    uint64_t rm_skipstart;      /* Column index of padding start */
    uint64_t rm_vector_mode;
    void *rm_datacopy;      /* rm_asize-buffer of copied data */
    uintptr_t rm_reports;       /* # of referencing checksum reports */
    uint8_t rm_freed;       /* map no longer has referencing ZIO */
    uint8_t rm_ecksuminjected;  /* checksum error was injected */
    raidz_col_t rm_col[1];      /* Flexible array of I/O columns */
} raidz_map_t;


extern const uint8_t vdev_raidz_pow2[256];
extern const uint8_t vdev_raidz_log2[256];

uint8_t
vdev_raidz_exp2(uint_t a, int exp);

void vdev_raidz_generate_parity_p_avx(raidz_map_t *rm);
int vdev_raidz_reconstruct_p_avx(raidz_map_t *rm, int *tgts, int ntgts);

void vdev_raidz_generate_parity_p_sse4(raidz_map_t *rm);
int vdev_raidz_reconstruct_p_sse4(raidz_map_t *rm, int *tgts, int ntgts);

void vdev_raidz_generate_parity_pq_avx(raidz_map_t *rm);
int vdev_raidz_reconstruct_pq_avx(raidz_map_t *rm, int *tgts, int ntgts);

void vdev_raidz_generate_parity_pq_sse4(raidz_map_t *rm);
int vdev_raidz_reconstruct_pq_sse4(raidz_map_t *rm, int *tgts, int ntgts);

int vdev_raidz_reconstruct_q_avx(raidz_map_t *rm, int *tgts, int ntgts);
int vdev_raidz_reconstruct_q_sse4(raidz_map_t *rm, int *tgts, int ntgts);

void vdev_raidz_generate_parity_pqr_avx(raidz_map_t *rm);
void vdev_raidz_generate_parity_pqr_sse4(raidz_map_t *rm);
#endif
