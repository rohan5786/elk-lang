#ifndef VALUE_H
#define VALUE_H

typedef double Value;

typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;

// macros to securely interpret and check different "Values" (str, char, num, vector)

void init_value_arr(ValueArray* arr);
void write_value_arr(ValueArray* arr, Value val);
void free_value_arr(ValueArray* arr);
void print_value(Value val);

#endif