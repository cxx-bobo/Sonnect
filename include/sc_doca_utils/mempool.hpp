#ifndef _SC_DOCA_UTILS_MEMPOOL_H_
#define _SC_DOCA_UTILS_MEMPOOL_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#include "sc_utils/list.hpp"

#if defined(SC_HAS_DOCA)

#include <doca_buf.h>

#define CACHE_LINE_SIZE     64

/** 
 * Align a value to a given power of two (get round)
 * @result
 *      Aligned value that is no bigger than value
 */
#define ALIGN_ROUND(val, align) \
    (decltype(val))((val) & (~((decltype(val))((align) - 1))))

/**
 * Align a value to a given power of two (get ceiling)
 * @result
 *      Aligned value that is no smaller than value
 */
#define ALIGN_CEIL(val, align) \
    ALIGN_ROUND(val + (decltype(val))((align) - 1), align)

/** 
 * Force alignment
 */
#define __aligned(a)        __attribute__((__aligned__(a)))

/*! 
 * \brief   mempool target structure
 */
struct mempool_target {
    /* entry for element list */
    struct list_entry entry;

    /* mempool this element belongs to */
    struct mempool *mp;

    /* the buffer this element points to */
    void *addr;

    /* doca buffer (metadata of the memory area) */
    struct doca_buf *buf;
};

/*! 
 * \brief   mempool structure 
 */
struct mempool {
    /* size of an element */
    uint64_t target_size;

    /* address of memory pool */     
    uint8_t *addr;

    /* size of memory pool */
    uint64_t size;

    /* list of free objects */
    struct list_entry target_free_list;

    /* list of used objects */
    struct list_entry target_used_list;
};

struct mempool* mempool_create(uint64_t num_target, uint64_t target_size);
bool is_mempool_full(struct mempool *mp);
void mempool_free(struct mempool *mp);
uint8_t mempool_get(struct mempool *mp, struct mempool_target **result_target);

#endif /* SC_HAS_DOCA */

#endif  /* _SC_DOCA_UTILS_MEMPOOL_H_ */