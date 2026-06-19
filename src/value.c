#include "value.h"
#include "memory.h"
#include "vector.h"

#include <stdio.h>

void init_value_arr(ValueArray* arr) {
  arr->capacity = 0;
  arr->count = 0;
  arr->values = NULL;
}

void write_value_arr(ValueArray* arr, Value val) {
  if (arr->capacity < arr->count + 1) {
    int old_capacity = arr->capacity;
    arr->capacity = ADD_CAPACITY(old_capacity);
    arr->values = ADD_ARR(Value, arr->values, arr->capacity);
  }

  arr->values[arr->count] = val;
  arr->count++;
}

void free_value_arr(ValueArray* arr) {
  ADD_ARR(Value, arr->values, 0);
  init_value_arr(arr);
}

void print_value(Value val) {
  switch (val.type) {
    case VAL_NULL: {
      printf("NULL");
      break;
    }
    case VAL_NUM: {
      printf("%g", val.as.num);
      break;
    }
    case VAL_CHAR: {
      printf("'%c'", val.as.ch);
      break;
    }
    case VAL_STR: {
      printf("%s", val.as.str);
      break;
    }
    case VAL_VEC: {
      Vector* vec = GET_VEC(val);
      printf("[");
      for (int i = 0; i < vec->count; i++) {
        print_value(vec->items[i]);
        if (i < vec->count - 1) printf(", ");
      }
      printf("]");
    }
  }
}

void print_err_value(Value val) {
  switch (val.type) {
    case VAL_NULL: {
      fprintf(stderr, "NULL");
      break;
    }
    case VAL_NUM: {
      fprintf(stderr, "%g", val.as.num);
      break;
    }
    case VAL_CHAR: {
      fprintf(stderr, "'%c'", val.as.ch);
      break;
    }
    case VAL_STR: {
      fprintf(stderr, "%s", val.as.str);
      break;
    }
    case VAL_VEC: {
      Vector* vec = GET_VEC(val);
      fprintf(stderr, "[");
      for (int i = 0; i < vec->count; i++) {
        print_value(vec->items[i]);
        if (i < vec->count - 1) fprintf(stderr, ", ");
      }
      fprintf(stderr, "]");
    }
  }
}