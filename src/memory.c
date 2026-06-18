#include "memory.h"

#include <stddef.h>
#include <stdlib.h>

#include "value.h"

void* reallocate(void* pointer, size_t new_size) {
  if (new_size == 0) {
    free(pointer);  // freeing the space for arr
    return NULL;    // safety
  }

  void* result =
      realloc(pointer, new_size);  // pointer to new size area in memory
  if (result == NULL) exit(1);     // err if no memory left
  return result;
}

bool values_equal(Value a, Value b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_NULL:
      return true;
    case VAL_NUM:
      return a.as.num == b.as.num;
    case VAL_CHAR:
      return a.as.ch == b.as.ch;
    case VAL_STR:
      return strcmp(a.as.str, b.as.str) == 0;  // convenience
    case VAL_VEC:
      return veccmp(GET_VEC(a), GET_VEC(b)) != 0;
  }
  return true;
}

// void *reallocate2(void *pointer, size_t new_size) {
//     free(pointer);
//     if (new_size == 0) return NULL;

//     void *result = malloc(new_size);
//     if (result == NULL) exit(1);
//     return result;
// }
