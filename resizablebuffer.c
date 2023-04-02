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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "resizablebuffer.h"
#include "bufferqueue.h"

/* default block size of the resizable buffer. */
#define RBUF_DEF_BLOCK_SIZE     512

/* default maximum size of the resizable buffer. */
#define RBUF_DEF_SIZE_MAX       1024

/* context of the resizable buffer. */
struct _rbuf_ctx {
    bque_ctx *bque;
    struct _rbuf_ctx_conf {
        rbuf_u32 block_size;
        rbuf_u32 size_max;
    } conf;
    struct _rbuf_ctx_cache {
        rbuf_u32 block_num;
        rbuf_u32 buff_cap;
        rbuf_u32 buff_size;
        rbuf_u32 last_block_buff_size;
    } cache;
};

/**
 * @brief create a resizable buffer.
 * 
 * @param ctx the address of the context pointer.
 * @param conf configuration pointer.
*/
rbuf_res rbuf_new(rbuf_ctx **ctx, rbuf_conf *conf) {
    rbuf_ctx *alloc_ctx;
    bque_ctx *mod_bque_ctx;
    bque_conf mod_bque_conf;
    bque_res mod_bque_res;

    RBUF_ASSERT(ctx != NULL);

    mod_bque_conf.buff_num_max = 0;
    mod_bque_conf.buff_size_max = 0;
    mod_bque_res = bque_new(&mod_bque_ctx, &mod_bque_conf);
    if (mod_bque_res != BQUE_OK) {
        if (mod_bque_res == BQUE_ERR_NO_MEM) {
            return RBUF_ERR_NO_MEM;
        } else {
            return RBUF_ERR;
        }
    }

    alloc_ctx = (rbuf_ctx *)malloc(sizeof(rbuf_ctx));
    if (alloc_ctx == NULL) {
        bque_del(mod_bque_ctx);

        return RBUF_ERR_NO_MEM;
    }
    memset(alloc_ctx, 0, sizeof(rbuf_ctx));

    alloc_ctx->bque = mod_bque_ctx;
    if (conf != NULL) {
        alloc_ctx->conf.block_size = conf->block_size;
        alloc_ctx->conf.size_max = conf->size_max;
    } else {
        alloc_ctx->conf.block_size = RBUF_DEF_BLOCK_SIZE;
        alloc_ctx->conf.size_max = RBUF_DEF_SIZE_MAX;
    }

    *ctx = alloc_ctx;

    return RBUF_OK;
}

/**
 * @brief delete the resizable buffer.
 * 
 * @param ctx context pointer.
*/
rbuf_res rbuf_del(rbuf_ctx *ctx) {
    RBUF_ASSERT(ctx != NULL);

    bque_del(ctx->bque);

    free(ctx);

    return RBUF_OK;
}

/**
 * @brief get the status of the resizable buffer.
 * 
 * @param ctx context pointer.
 * @param stat status pointer.
*/
rbuf_res rbuf_status(rbuf_ctx *ctx, rbuf_stat *stat) {
    RBUF_ASSERT(ctx != NULL);
    RBUF_ASSERT(stat != NULL);

    stat->block_num = ctx->cache.block_num;
    stat->buff_size = ctx->cache.buff_size;

    return RBUF_OK;
}

/**
 * @brief resize the buffer size of the resizable buffer.
 * 
 * @param ctx context pointer.
 * @param size the new size.
*/
rbuf_res rbuf_resize(rbuf_ctx *ctx, rbuf_u32 size) {
    rbuf_u32 new_block_num;
    rbuf_u32 block_num_diff;
    rbuf_u32 new_block_num_quotient;
    rbuf_u32 new_block_num_remainder;
    bque_res mod_bque_res;

    RBUF_ASSERT(ctx != NULL);

    /* check whether the size is too large. */
    if (size > ctx->conf.size_max) {
        return RBUF_ERR_BAD_SIZE;
    }

    new_block_num_quotient = size / ctx->conf.block_size;
    new_block_num_remainder = size % ctx->conf.block_size;
    new_block_num = new_block_num_quotient + ((new_block_num_remainder != 0) ? 1 : 0);
    if (new_block_num > ctx->cache.block_num) {
        block_num_diff = new_block_num - ctx->cache.block_num;
        for (rbuf_u32 i = 0; i < block_num_diff; i++) {
            mod_bque_res = bque_enqueue(ctx->bque, NULL, ctx->conf.block_size);
            if (mod_bque_res != BQUE_OK) {
                if (mod_bque_res == BQUE_ERR_NO_MEM) {
                    return RBUF_ERR_NO_MEM;
                } else {
                    return RBUF_ERR;
                }
            }
        }
        ctx->cache.block_num = new_block_num;
        ctx->cache.buff_cap = ctx->conf.block_size * new_block_num;
    } else if (new_block_num < ctx->cache.block_num) {
        block_num_diff = ctx->cache.block_num - new_block_num;
        for (rbuf_u32 i = 0; i < block_num_diff; i++) {
            mod_bque_res = bque_forfeit(ctx->bque, NULL, NULL);
            if (mod_bque_res != BQUE_OK) {
                return RBUF_ERR;
            }
        }

        ctx->cache.block_num = new_block_num;
        ctx->cache.buff_cap = ctx->conf.block_size * new_block_num;
    }

    /* update the buffer size of the whole resizable buffer. */
    ctx->cache.buff_size = size;

    /* update the buffer size of the last block. */
    if (new_block_num_quotient != 0 &&
        new_block_num_remainder == 0) {
        ctx->cache.last_block_buff_size = ctx->conf.block_size;
    } else {
        ctx->cache.last_block_buff_size = new_block_num_remainder;
    }

    return RBUF_OK;
}

/**
 * @brief copy external data into the resizable buffer.
 * 
 * @param ctx context pointer.
 * @param buff external buffer pointer.
 * @param offs offset indicating where to start copying in the resizable buffer.
 * @param size data copying size;
*/
rbuf_res rbuf_copy_from(rbuf_ctx *ctx, const void *buff, rbuf_u32 offs, rbuf_u32 size) {
    rbuf_u32 new_size;
    rbuf_u32 block_offs;
    rbuf_u32 starting_block_idx;
    rbuf_u32 ending_block_idx;
    bque_buff mod_bque_buff;
    bque_res mod_bque_res;
    rbuf_res res;

    RBUF_ASSERT(ctx != NULL);
    RBUF_ASSERT(buff != NULL);

    new_size = offs + size;

    /* if the new buffer size is greater
       than current buffer capacity, then
       resize the whole resizable buffer. */
    if (new_size > ctx->cache.buff_cap) {
        res = rbuf_resize(ctx, new_size);
        if (res != RBUF_OK) {
            return res;
        }
    }

    starting_block_idx = offs / ctx->conf.block_size;
    ending_block_idx = new_size / ctx->conf.block_size;
    if (starting_block_idx == ending_block_idx) {
        mod_bque_res = bque_item(ctx->bque, starting_block_idx, &mod_bque_buff);
        if (mod_bque_res != BQUE_OK) {
            return RBUF_ERR;
        }

        block_offs = offs % ctx->conf.block_size;
        memcpy(mod_bque_buff.ptr + block_offs, buff, size);
    } else {
        rbuf_u32 buff_offs;
        rbuf_u32 rest_size;
        rbuf_u32 curt_size;

        mod_bque_res = bque_item(ctx->bque, starting_block_idx, &mod_bque_buff);
        if (mod_bque_res != BQUE_OK) {
            return RBUF_ERR;
        }

        block_offs = offs % ctx->conf.block_size;
        curt_size = ctx->conf.block_size - block_offs;
        memcpy(mod_bque_buff.ptr + block_offs, buff, curt_size);

        buff_offs = curt_size;
        rest_size = size - curt_size;

        for (bque_u32 i = starting_block_idx + 1; i < ending_block_idx + 1; i++) {
            mod_bque_res = bque_item(ctx->bque, (bque_s32)i, &mod_bque_buff);
            if (mod_bque_res != BQUE_OK) {
                return RBUF_ERR;
            }

            if (rest_size > ctx->conf.block_size) {
                curt_size = ctx->conf.block_size;
            } else {
                curt_size = rest_size;
            }

            memcpy(mod_bque_buff.ptr, (rbuf_u8 *)buff + buff_offs, curt_size);

            buff_offs += curt_size;
            rest_size -= curt_size;
        }
    }

    return RBUF_OK;
}

/**
 * @brief append external data to the end of the resizable buffer.
 * 
 * @param ctx context pointer.
 * @param buff external buffer pointer.
 * @param size data appending size;
*/
rbuf_res rbuf_append(rbuf_ctx *ctx, const void *buff, rbuf_u32 size) {
    rbuf_res res;

    RBUF_ASSERT(ctx != NULL);
    RBUF_ASSERT(buff != NULL);

    res = rbuf_copy_from(ctx, buff, ctx->cache.buff_size, size);
    return res;
}

/**
 * @brief copy data in the resizable buffer into the external buffer.
 * 
 * @param ctx context pointer.
 * @param buff external buffer pointer.
 * @param offs offset indicating where to start copying in the resizable buffer.
 * @param size data copying size;
*/
rbuf_res rbuf_copy_to(rbuf_ctx *ctx, void *buff, rbuf_u32 offs, rbuf_u32 size) {
    rbuf_u32 ending_byte_no;
    rbuf_u32 block_offs;
    rbuf_u32 starting_block_idx;
    rbuf_u32 ending_block_idx;
    bque_buff mod_bque_buff;
    bque_res mod_bque_res;

    RBUF_ASSERT(ctx != NULL);
    RBUF_ASSERT(buff != NULL);

    if (offs > ctx->cache.buff_size) {
        return RBUF_ERR_BAD_OFFS;
    }

    ending_byte_no = offs + size;
    if (ending_byte_no > ctx->cache.buff_size) {
        return RBUF_ERR_BAD_SIZE;
    }

    starting_block_idx = offs / ctx->conf.block_size;
    ending_block_idx = ending_byte_no / ctx->conf.block_size;
    if (starting_block_idx == ending_block_idx) {
        mod_bque_res = bque_item(ctx->bque, starting_block_idx, &mod_bque_buff);
        if (mod_bque_res != BQUE_OK) {
            return RBUF_ERR;
        }

        block_offs = offs % ctx->conf.block_size;
        memcpy(buff, mod_bque_buff.ptr + block_offs, size);
    } else {
        rbuf_u32 buff_offs;
        rbuf_u32 rest_size;
        rbuf_u32 curt_size;

        mod_bque_res = bque_item(ctx->bque, starting_block_idx, &mod_bque_buff);
        if (mod_bque_res != BQUE_OK) {
            return RBUF_ERR;
        }

        block_offs = offs % ctx->conf.block_size;
        curt_size = ctx->conf.block_size - block_offs;
        memcpy(buff, mod_bque_buff.ptr + block_offs, curt_size);

        buff_offs = curt_size;
        rest_size = size - curt_size;

        for (bque_u32 i = starting_block_idx + 1; i < ending_block_idx + 1; i++) {
            mod_bque_res = bque_item(ctx->bque, (bque_s32)i, &mod_bque_buff);
            if (mod_bque_res != BQUE_OK) {
                return RBUF_ERR;
            }

            if (rest_size > ctx->conf.block_size) {
                curt_size = ctx->conf.block_size;
            } else {
                curt_size = rest_size;
            }

            memcpy((rbuf_u8 *)buff + buff_offs, mod_bque_buff.ptr, curt_size);

            buff_offs += curt_size;
            rest_size -= curt_size;
        }
    }

    return RBUF_OK;
}
