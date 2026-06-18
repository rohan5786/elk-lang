#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>

typedef enum {
  VAL_NULL,
  VAL_NUM,
  VAL_STR,
  VAL_CHAR,
  VAL_VEC,
} ValType;

struct Vector; // forward dec

typedef struct Value {
  ValType type;
  union {
    double num;
    char* str;
    char ch;
    struct Vector* vec;
  } as;
} Value;

// creating values
#define NUM_VAL(value) ((Value) {VAL_NUM, {.num = value}})
#define NULL_VAL ((Value) {VAL_NULL, {.num = 0}})
#define STR_VAL(value) \
  ((Value){            \
      VAL_STR,         \
      {.str = value}})  // auto decay to char* if "", otherwise pass char*
#define VEC_VAL(value) \
  ((Value){VAL_VEC, {.vec = value}})  // pass Vector* made w/malloc

// getting values
#define GET_NUM(value) ((value).as.num)
#define GET_CHAR(value) ((value).as.ch)
#define GET_STR(value) ((value).as.str)
#define GET_VEC(value) ((value).as.vec)

// checking values
#define IS_NUM(value) ((value).type == VAL_NUM)
#define IS_CHAR(value) ((value).type == VAL_CHAR)
#define IS_STR(value) ((value).type == VAL_STR)
#define IS_VEC(value) ((value).type == VAL_VEC)
#define IS_NULL(value) ((value).type == VAL_NULL)

typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;

// macros to securely interpret and check different "Values" (str, char, num,
// vector)

void init_value_arr(ValueArray* arr);
void write_value_arr(ValueArray* arr, Value val);
void free_value_arr(ValueArray* arr);
void print_value(Value val);

#endif
