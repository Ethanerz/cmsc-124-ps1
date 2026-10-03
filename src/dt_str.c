/*
 * dt_str.c: Length-carrying strings for Unit 5, Section B.
 *
 * A C string is a null-terminated character sequence stored in an array.
 * An array expression usually converts to a pointer to its first character.
 * strlen reads only through the first zero byte.
 * A pointer does not store the array capacity.
 *
 * This type stores the length and capacity with the bytes. dt_str_len reads a
 * field. A zero byte is data. Append operations use the stored capacity.
 *
 * An implementation can store a final zero byte after the data.
 * The public interface requires callers to use dt_str_len.
 */

#include "dt.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct dt_str {
    char  *bytes;
    size_t length;
    size_t capacity;
};

/*
 * dt_str_new copies the first `length` bytes. A zero byte is data. The function
 * returns NULL when allocation or size representation fails.
 */
dt_str *dt_str_new(const char *bytes, size_t length)
{
{
    if (length == SIZE_MAX) {
        return NULL;                      
    }

    dt_str *s = malloc(sizeof *s);
    if (s == NULL) {
        return NULL;
    }

    s->capacity = length + 1;             /* +1 for the terminator */
    s->bytes = malloc(s->capacity);
    if (s->bytes == NULL) {
        free(s);                         
        return NULL;
    }

    if (length > 0) {
        memcpy(s->bytes, bytes, length);  /* memcpy, not strcpy: zero bytes are data */
    }
    s->bytes[length] = '\0';
    s->length = length;
    return s;
}
}

/*
 * dt_str_free releases the buffer and handle. It accepts NULL.
 */
void dt_str_free(dt_str *s)
{
    if (s == NULL) {
        return;
    }
    free(s->bytes);
    free(s);
}
/*
 * dt_str_len returns the stored byte count in constant time.
 */
size_t dt_str_len(const dt_str *s)
{
    return s->length;
}

/*
 * dt_str_bytes returns the string bytes. Internal storage can include a final
 * zero byte. Callers must use dt_str_len with this pointer.
 */
const char *dt_str_bytes(const dt_str *s)
{
    return s->bytes;
}

/*
 * dt_str_append adds `length` bytes and grows the buffer when necessary. It
 * returns DT_ERR_CAPACITY when allocation or size representation fails.
 * The function does not change the string after a failure.
 */
dt_status dt_str_append(dt_str *s, const char *bytes, size_t length)
{
    /* Room check first, phrased so it can't wrap: length + s->length + 1 <= SIZE_MAX */
    if (length > SIZE_MAX - 1 - s->length) {
        return DT_ERR_CAPACITY;
    }
    size_t needed = s->length + length + 1;

    if (needed > s->capacity) {
        size_t new_cap = s->capacity;
        while (new_cap < needed) {
            if (new_cap > SIZE_MAX / 2) {  /* doubling would wrap */
                new_cap = needed;
                break;
            }
            new_cap *= 2;
        }
        char *tmp = realloc(s->bytes, new_cap);   /* temp pointer: old block stays valid on failure */
        if (tmp == NULL) {
            return DT_ERR_CAPACITY;               /* s is unchanged */
        }
        s->bytes = tmp;
        s->capacity = new_cap;
    }

    if (length > 0) {
        memcpy(s->bytes + s->length, bytes, length);
    }
    s->length += length;
    s->bytes[s->length] = '\0';					  /* add terminator to end of new string  */
    return DT_OK;
}

/*
 * dt_str_substr builds a new string from length bytes at start.
 * It returns DT_ERR_RANGE when the requested range exceeds the source.
 * It returns DT_ERR_CAPACITY after an allocation failure.
 * The function does not change the source string.
 */
dt_status dt_str_substr(const dt_str *s, size_t start, size_t length, dt_str **out)
{
    if (start > s->length) {
        return DT_ERR_RANGE;
    }
    if (length > s->length - start) {         /* safe: start <= length was just checked */
        return DT_ERR_RANGE;
    }

    dt_str *piece = dt_str_new(s->bytes + start, length);
    if (piece == NULL) {
        return DT_ERR_CAPACITY;
    }
    *out = piece;                             /* only written on success */
    return DT_OK;
}

/*
 * dt_str_eq reports whether both strings hold the same bytes.
 * The stored lengths let the comparison include embedded zero bytes.
 */
bool dt_str_eq(const dt_str *a, const dt_str *b)
{
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->bytes, b->bytes, a->length) == 0;
}
