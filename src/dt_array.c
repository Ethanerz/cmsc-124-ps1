/*
 * dt_array.c: Array descriptors for Unit 5, Section D.
 *
 * Built-in C arrays use offsets that start at zero. Ada, Fortran, and Pascal
 * can use bounds such as 1..10 or -5..5. The index and offset then differ.
 *
 *     offset = index - lower_bound
 *
 * The run-time descriptor stores the lower bound for this subtraction.
 * The mathematical difference can exceed long long.
 * Confirm that the result is representable before you subtract signed values.
 *
 * Check both bounds. An index below the lower bound can access memory before
 * the allocation. That access has undefined behavior.
 */

#include "dt.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

struct dt_array {
    dt_value *elements;
    size_t    length;
    long long lower_bound;
};

/*
 * dt_array_new builds an array of length nil elements.
 * The first index is lower_bound. A zero length creates a valid empty array.
 * It returns NULL for an invalid size, invalid index range, or allocation failure.
 */
dt_array *dt_array_new(size_t length, long long lower_bound)
{
    /* TODO: Allocate the descriptor and length elements.
        Set each element to dt_value_nil(). Store the lower bound.
        Return a valid array for a zero length.
        Reject a nonempty range with an unrepresentable final index.
        Reject an element block size that exceeds SIZE_MAX.
        dt_array_new(3, 0)   -> three nil elements, indices 0, 1, 2
        dt_array_new(3, -1)  -> three nil elements, indices -1, 0, 1
        dt_array_new(0, 0)   -> an empty array
        cases/normal/array_basics.case, cases/boundary/array_empty.case,
       cases/boundary/array_negative_lower_bound.case */
    
    // rejects an element block size that exceeds SIZE_MAX
    // we divide the SIZE_MAX by the size of each element to know how much we can allocate for each
    // a length that is larger than this allocation is rejected
    if (length > (SIZE_MAX / sizeof(dt_value))) {
        return NULL;
    }

    // check if offset can be represented in long long
    if (length > 0) {
        size_t last_offset = length - 1;
        if (last_offset > LLONG_MAX) {
            return NULL;    // reject if it cannot fit
        }

        // if it can, solve for final index
        long long final_index;
        dt_status add = dt_int_add(lower_bound, (long long)last_offset, &final_index);
        if (add != DT_OK) {
            return NULL;    // reject if final index is not representable
        }
    }

    // allocation
    // allocate enough memory for one dt_array struct
    dt_array *a = malloc(sizeof(*a));
        if (a == NULL) {
            return NULL;    // if no memory was allocated, report allocation failure
    }

    // allocate enough memory for one elements array
    dt_value *elements;
    if (length == 0) {
        elements = NULL;    // sets a valid array to null (since length is 0)
    } else {
        elements = malloc(length * sizeof(dt_value));
        // if array has no elements, free the memorty allocated for the struct above
        // then report allocation failure
        if (elements == NULL) {
            free(a);
            return NULL;
        }
    }
    
    // set value of each memory slot to nil
    for (size_t i = 0; i < length; i++) {
        elements[i] = dt_value_nil();
    }

    // assign corresponding values to each struct fields
    a->length =         length;
    a->elements =       elements;
    a->lower_bound =    lower_bound;
    
    // return the complete struct
    return a;
}

/*
 * dt_array_free releases the element block and descriptor. It accepts NULL.
 * The environment owns the runtime objects referenced by the dt_value elements.
 */
void dt_array_free(dt_array *a)
{
    /* TODO: Release the elements. Then release the descriptor.
        Preserve the referenced values. The driver environment owns them.
        an array holding a string  -> the element block goes, the string stays
        dt_array_free(NULL)        -> returns, having done nothing */
    
    // a NULL argument returns, does nothing
    if (a == NULL) {
        return;
    }

    free(a->elements);  // release elements first
    free(a);            // release the entire descriptor next

}

/*
 * dt_array_len returns the stored element count in constant time.
 */
size_t dt_array_len(const dt_array *a)
{
    /* TODO: Return the stored length. The lower bound does not affect it.
        after `arr new a 3 0`:   dt_array_len(a) -> 3
        after `arr new a 3 -1`:  dt_array_len(a) -> 3, the same three elements
        after `arr new a 0 0`:   dt_array_len(a) -> 0
       cases/normal/array_basics.case, cases/boundary/array_empty.case */
        return a->length;
}

/*
 * dt_array_lower_bound returns the first array index. With lower bound 1,
 * index 1 uses storage offset 0.
 */
long long dt_array_lower_bound(const dt_array *a)
{
    /* TODO: Return the lower bound that the constructor stored.
        dt_array_get uses this value to calculate an element offset.
        after `arr new a 3 -1`:  dt_array_lower_bound(a) -> -1
        after `arr new a 3 1`:   dt_array_lower_bound(a) -> 1
        cases/boundary/array_negative_lower_bound.case,
       cases/boundary/array_lower_bound_one.case */
        return a->lower_bound;
}

/*  
 *  helper function to check bounds and then solve for offset
 *  returns DT_ERR_RANGE if undefined behavior, DT_OK if within bounds
 *  it also sets offset to calculated distance within the function
 */
static dt_status array_offset(const dt_array *a, long long index, size_t *offset) {
    
    // an index below the bound might access storage outside the allocation, which is undefined behavior
    // check if index is below the lower-bound of the array
    if (index < dt_array_lower_bound(a)) {
        return DT_ERR_RANGE;   // reject invalid indices
    }

    // index >= a->lower_bound, so we can now solve for the offset
    /*casting everything to ULL (has bigger nonnegative range than LL)
    so subtraction won't trigger a *signed* overflow */
    unsigned long long ull_index = index;
    unsigned long long ull_lb = dt_array_lower_bound(a);
    unsigned long long distance = ull_index - ull_lb;
    
    // an index (offset) above the bound (length) might access storage outside the allocation, which is undefined behavior
    // check if index (offset) is above the upper-bound of the array
    if (distance >= a->length) {
        return DT_ERR_RANGE;
    }
    *offset = (size_t)distance;
    return DT_OK;
}

/*
 * dt_array_get writes the element at index to *out.
 * It returns DT_ERR_RANGE and does not change *out for an invalid index.
 */
dt_status dt_array_get(const dt_array *a, long long index, dt_value *out)
{
    /* TODO: Reject an index below the lower bound.
        Calculate the nonnegative distance without signed overflow.
        Reject a distance that is at least the length.
        Convert the checked distance to size_t for the element offset.
        an array over -1..1:
        dt_array_get(a, -1, &out)  -> DT_OK, offset 0
        dt_array_get(a,  1, &out)  -> DT_OK, offset 2
        dt_array_get(a,  2, &out)  -> DT_ERR_RANGE, *out untouched
        dt_array_get(a, -2, &out)  -> DT_ERR_RANGE, below the lower bound
        cases/boundary/array_index_above_upper.case,
        cases/boundary/array_index_below_lower.case,
       cases/boundary/array_full_range_index.case */
    
    size_t offset;      // initialize size_t offset for conversion

    // calls helper function array_offset
    // returns either DT_ERR_RANGE if out of bounds or DT_OK if within bounds
    // sets offset variable to calculated distance within the function
    dt_status check = array_offset(a, index, &offset);
    
    // returns DT_ERR_RANGE; rejects index below lower bound or distance that is at least the length
    if (check != DT_OK) {
        return check;
    }

    // means distance is within bounds
    *out = a->elements[offset];     // writes element at offset to out
    return check;                   // DT_OK
}

/*
 * dt_array_set replaces the element at index with v.
 * It returns DT_ERR_RANGE and changes nothing for an invalid index.
 * The environment keeps ownership of the old value.
 */
dt_status dt_array_set(dt_array *a, long long index, dt_value v)
{
    /* TODO: Use the same bounds check as dt_array_get. Then write the value.
        Put the shared check in one helper.
        an array over -1..1:
        dt_array_set(a, -1, dt_value_int(10))  -> DT_OK, offset 0 holds 10
        dt_array_set(a,  2, dt_value_int(10))  -> DT_ERR_RANGE, nothing changes
        cases/normal/array_basics.case, cases/boundary/array_negative_lower_bound.case */

    size_t offset;      // initialize size_t offset for conversion

    // calls helper function array_offset to check bounds and calculate offset
    // returns either DT_ERR_RANGE if out of bounds or DT_OK if within bounds
    dt_status check = array_offset(a, index, &offset);
    
    // returns DT_ERR_RANGE; rejects index below lower bound or distance that is at least the length
    if (check != DT_OK) {
        return check;
    }

    // means distance is within bounds
    a->elements[offset] = v;     // writes element at offset to out
    return check;
    
}
