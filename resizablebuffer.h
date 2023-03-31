/**
 * MIT License
 * 
 * Copyright (c) 2023 Alex Chen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __RBUF_H__
#define __RBUF_H__

#include <stdint.h>

#ifdef RBUF_DEBUG

#include <stdarg.h>
#include <stdio.h>

#endif

/* basic data types. */
typedef int8_t      rbuf_s8;
typedef uint8_t     rbuf_u8;

typedef int16_t     rbuf_s16;
typedef uint16_t    rbuf_u16;

typedef int32_t     rbuf_s32;
typedef uint32_t    rbuf_u32;

typedef int64_t     rbuf_s64;
typedef uint64_t    rbuf_u64;

enum _rbuf_res {

    /* all is well, so far :) */
    RBUF_OK             = 0,

    /* generic error occurred. */
    RBUF_ERR            = -1,

    /* failed to allocate memory. */
    RBUF_ERR_NO_MEM     = -2,

    /* invalid offset. */
    RBUF_ERR_BAD_OFFS   = -3,

    /* invalid size. */
    RBUF_ERR_BAD_SIZE   = -4,
};

#ifdef RBUF_DEBUG

/* logging function for debug. */
#define RBUF_LOG(fmt, ...)  printf(fmt, ##__VA_ARGS__)

/* assertion macro used in the APIs. */
#define RBUF_ASSERT(expr)   \
    if (!(expr)) { RBUF_LOG("[RBUF] %s:%d: assertion failed: \"%s\"\n", \
        __FILE__, __LINE__, #expr); while (1);};

/* returned result used in the APIs. */
typedef enum _rbuf_res      rbuf_res;

#else

/* returned result used in the APIs. */
typedef rbuf_s32            rbuf_res;

/* assertion macro used in the APIs. */
#define RBUF_ASSERT(expr)

#endif

/* configuration of the resizable buffer. */
typedef struct _rbuf_conf {
    rbuf_u32 block_size;
    rbuf_u32 size_max;
} rbuf_conf;

/* status of the resizable buffer. */
typedef struct _rbuf_stat {
    rbuf_u32 block_num;
    rbuf_u32 buff_size;
} rbuf_stat;

/* context of the resizable buffer. */
typedef struct _rbuf_ctx    rbuf_ctx;

rbuf_res rbuf_new(rbuf_ctx **ctx, rbuf_conf *conf);

rbuf_res rbuf_del(rbuf_ctx *ctx);

rbuf_res rbuf_status(rbuf_ctx *ctx, rbuf_stat *stat);

rbuf_res rbuf_resize(rbuf_ctx *ctx, rbuf_u32 size);

rbuf_res rbuf_copy_from(rbuf_ctx *ctx, const void *buff, rbuf_u32 offs, rbuf_u32 size);

rbuf_res rbuf_copy_to(rbuf_ctx *ctx, void *buff, rbuf_u32 offs, rbuf_u32 size);

#endif
