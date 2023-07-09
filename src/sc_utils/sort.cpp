#include "sc_global.hpp"
#include "sc_utils.hpp"
#include "sc_control_plane.hpp"

int __internal_merge_sort_long(long *list, uint64_t low, uint64_t high, bool is_increament);
short __merge_long(long *list, uint64_t low, uint64_t high, bool is_increament);

/*!
 * \brief merge_sort sort for long list
 * \param   list    list to be sorted
 * \param   length  length of the list
 * \param   is_increament   whether use the increament order
 * \return  sort status
 */
int sc_util_merge_sort_long(long *list, uint64_t length, bool is_increament){
    /*!
     * [Note]
     * 1. Call stack:
     *                          __internal_merge_sort(0, length-1)
     *                          /                                \
     *                         /                                  \
     *                        /                                    \
     *        __internal_merge_sort(0, mid)         __internal_merge_sort(mid, length-1)
     *                        \                                    /
     *                         \                                  /
     *                          \                                /
     *                                 __merge_(0, length-1)
     */
    return __internal_merge_sort_long(list, 0, length-1, is_increament);
}

/*!
 * \brief recursive function for long merge sort
 * \param   list    list to be sorted
 * \param   low     the low index of the list to be sorted
 * \param   high    the high index of the list to be sorted
 * \param   is_increament   whether use the increament order
 * \return  sort status
 */
int __internal_merge_sort_long(long *list, uint64_t low, uint64_t high, bool is_increament){
     uint64_t mid;
    
    // recursive quit condition
    if(low == high){
        goto low_eq_high;
    }

    // obtain middle position
    mid = (low+high)/2;

    // recursive calling to sort two sublists
    if(__internal_merge_sort_long(list, low, mid, is_increament) != SC_SUCCESS) 
        return SC_ERROR_INTERNAL;
    if(__internal_merge_sort_long(list, mid+1, high, is_increament) != SC_SUCCESS) 
        return SC_ERROR_INTERNAL;

    // merge two sorted sublists
    return __merge_long(list, low, high, is_increament);

low_eq_high:
    return SC_SUCCESS;
}

/*!
 * \brief merge two sorted sublists with "equal" lengthes (maybe larger than 1)
 * \param   list    list to be merged
 * \param   low     the low index of the list to be merged
 * \param   high    the high index of the list to be merged
 * \param   is_increament   whether use the increament order
 * \return  sort status
 */
short __merge_long(long *list, uint64_t low, uint64_t high, bool is_increament){
    // obtain middle position
    uint64_t mid = (low+high)/2;

    // malloc space
    long *temp_list = (long*)malloc(sizeof(long)*(high-low+1));
    if(unlikely(!temp_list)){
            SC_THREAD_ERROR_DETAILS("failed to allocate memory for temp list");
            return SC_ERROR_MEMORY;
    }

    uint64_t a_index=low, b_index=mid+1, temp_index=0;

    while(a_index<=mid && b_index<=high){
        // this is a XNOR operator, the truth table is:
        // | is_increament | a is larger | opeartion |
        // |      T        |      T      |   use a   |
        // |      F        |      F      |   use a   |
        // |      T        |      F      |   use b   |
        // |      F        |      T      |   use b   |
        if(is_increament != (list[a_index] > list[b_index])){
            temp_list[temp_index] = list[a_index];
            a_index++;
        } else {
            temp_list[temp_index] = list[b_index];
            b_index++;
        }
        temp_index++;
    }

    while(a_index<=mid){
        temp_list[temp_index] = list[a_index];
        a_index++;
        temp_index++;
    }

    while(b_index<=high){
        temp_list[temp_index] = list[b_index];
        b_index++;
        temp_index++;
    }

    for(uint64_t i=0; i<(high-low+1); i++){
        list[low+i] = temp_list[i];
    }

    free(temp_list);
    return SC_SUCCESS;
}