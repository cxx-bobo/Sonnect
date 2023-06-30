#include "sc_log.hpp"
#include "sc_utils.hpp"
#include "sc_doca_utils/mempool.hpp"

#if defined(SC_HAS_DOCA)

/*!
 * \brief   create a new memory pool, according to number of elements, and element size
 * \note    the elements will be allocated within continuous memory area
 * \param   num_target      number of elements within the memory pool
 * \param   target_size     the size of a single element
 * \return  the created memory pool objects
 */
struct mempool* mempool_create(uint64_t num_target, uint64_t target_size) {
    uint64_t total_size = num_target * target_size;
    struct mempool_target *targets;
    uint64_t i;

    // allocate the memory pool struct itself
    struct mempool *mp = (struct mempool *)malloc(sizeof(struct mempool));
    if (!mp) {
        SC_THREAD_ERROR(
            "failed to allocate memory pool with %lu elements, each in size of %lu bytes",
            num_target, target_size
        )
        goto mempool_create_exit;
    }

    // allocate a continuous memory region
    mp->addr = (uint8_t *)calloc(num_target, target_size);
    if (!mp->addr) {
        SC_THREAD_ERROR(
            "failed to allocate continuous memory inside memory pool with %lu elements, each in size of %lu bytes",
            num_target, target_size
        )
        goto mempool_create_free_mp;
    }
    mp->target_size = target_size;
    mp->size = total_size;

    // initialize two link list indicator within the memory pool
    init_list_head(&mp->target_free_list);
    init_list_head(&mp->target_used_list);

    // allocate the memory pool elements' struct (not data)
    targets = (struct mempool_target *)calloc(num_target, sizeof(struct mempool_target));
    if (!targets) {
        SC_THREAD_ERROR("failed to allocate continuous memory for %lu memory pool element structs", num_target)
        goto mempool_create_free_mp_buf;
    }

    // segment the region into pieces (initialize each element)
    for (i=0; i<num_target; i++) {
        targets[i].mp = mp;
        targets[i].addr = mp->addr + i*target_size;
        list_add_tail(&targets[i].entry, &mp->target_free_list);
    }

    return mp;

mempool_create_free_mp_buf:
    free(mp->addr);

mempool_create_free_mp:
    free(mp);

mempool_create_exit:
    return mp;
}

/*!
 * \brief   judge whether a memory pool is full (i.e., no element inside free list)
 * \param   mp  the memory pool to be judge
 * \return  judge result
 */
bool is_mempool_empty(struct mempool *mp) {
    if (list_empty(&mp->target_free_list)) {
        return true;
    }
    return false;
}

/*!
 * \brief   free the memory area occupied by the mempool
 * \param   mp  the memory pool to be freed
 */
void mempool_free(struct mempool *mp) {
    struct mempool_target *target, *temp_target;
    struct list_entry head;

    LIST_FOR_EACH_SAFE(
        target, temp_target, &(mp->target_free_list), struct mempool_target
    ){
        free(target);
    }

    LIST_FOR_EACH_SAFE(
        target, temp_target, &(mp->target_used_list), struct mempool_target
    ){
        free(target);
    }

    free(mp->addr);
    free(mp);
    
    return;
}

/*!
 * \brief   get a free mempool_target from the mempool (if exists)
 * \param   mp      the source memory pool
 * \param   target  the obtained mempool target
 */
uint8_t mempool_get(struct mempool *mp, struct mempool_target **result_target) {
    struct mempool_target *target 
        = LIST_FIRST_TARGET_OR_NULL(&mp->target_free_list, struct mempool_target);
    if (!target) {
        *result_target = NULL;
        return SC_ERROR_NOT_EXIST;
    }
    
    list_del_init(&target->entry);
    *result_target = target;
    return SC_SUCCESS;
}

/*!
 * \brief   put a element to the mempool
 * \param   mp      the destination memory pool
 * \param   target  the mempool target to append
 */
void mempool_put(struct mempool *mp, struct mempool_target *target) {
    list_add_tail(&(target->entry), &mp->target_free_list);
}

#endif /* SC_HAS_DOCA */