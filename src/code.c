#include "code.h"
#include "memory.h"
#include "value.h"
#include <stdlib.h>

void init_code(Code *code) {
  code->capacity = 0;
  code->count = 0;
  code->bytes = NULL;
  code->line_capacity = 0;
  code->line_count = 0;
  code->lines = NULL;
  code->instruction_counts = NULL;
  init_value_arr(&code->constants); // &code->constants == &((*code).constants) (passing pointer to ValueArray)
}

void write_code(Code *code, uint8_t new_byte, int line) {
  // not enough space for one more byte, double space currently
  if (code->capacity < code->count + 1) {
    const int old_capacity = code->capacity;
    code->capacity = ADD_CAPACITY(old_capacity);
    code->bytes = ADD_ARR(uint8_t, code->bytes, code->capacity);
  }
  code->bytes[code->count] = new_byte;
  code->count++;

  if (code->line_capacity > 0) {
    if (code->lines[code->line_count - 1] == line)
      code->instruction_counts[code->line_count - 1]++;
    else {
      if (code->line_capacity < code->line_count + 1) {
        const int old_capacity = code->line_capacity;
        code->line_capacity = ADD_CAPACITY(old_capacity);
        code->lines = ADD_ARR(int, code->lines, code->line_capacity);
        code->instruction_counts =
            ADD_ARR(int, code->instruction_counts, code->line_capacity);
      }
      code->lines[code->line_count] = line;
      code->instruction_counts[code->line_count] = 1;
      code->line_count++;
    }
  } else {
    code->line_capacity = ADD_CAPACITY(0); // old_capacity = 0
    code->lines = ADD_ARR(int, code->lines, code->line_capacity);
    code->instruction_counts =
        ADD_ARR(int, code->instruction_counts, code->line_capacity);
    // quicky update
    code->line_count = 1;
    code->lines[code->line_count - 1] = line;
    code->instruction_counts[code->line_count - 1] = 1;
  }
}

int add_constant(Code *code, Value val) {
  write_value_arr(&code->constants, val);
  return code->constants.count - 1;
}

// TODO: implement i64 compatibility (another 32-bit value arr?) 
// REFACTOR
void write_constant(Code *code, Value val, int line) {
  int index = add_constant(code, val);
  const uint8_t mask = 0b11111111;

  if (index < 128) {
    write_code(code, OP_I8, line);
    
    write_code(code, index, line);
  }
  else if (index < 65536) {
    write_code(code, OP_I16, line);
    
    write_code(code, index, line);
    write_code(code, (uint8_t) (index >> 8) & mask, line);
  }
  else {
    write_code(code, OP_I32, line);

    write_code(code, index, line);
    write_code(code, (uint8_t) (index >> 8) & mask, line);
    write_code(code, (uint8_t) (index >> 16) & mask, line);
    write_code(code, (uint8_t) (index >> 24) & mask, line);
  }
  // won't work cuz index too small
  // else {
  //   write_code(code, OP_I64, line);
  //   write_code(code, index, line);
  //   write_code(code, (uint8_t) (index >> 8) & mask, line);
  //   write_code(code, (uint8_t) (index >> 16) & mask, line);
  //   write_code(code, (uint8_t) (index >> 24) & mask, line);
  //   write_code(code, (uint8_t) (index >> 32) & mask, line);
  //   write_code(code, (uint8_t) (index >> 40) & mask, line);
  //   write_code(code, (uint8_t) (index >> 48) & mask, line);
  //   write_code(code, (uint8_t) (index >> 56) & mask, line);
  // }
}

void free_code(Code *code) {
  ADD_ARR(uint8_t, code->bytes, 0); // new size is 0
  ADD_ARR(int, code->lines, 0);
  ADD_ARR(int, code->instruction_counts, 0);
  free_value_arr(&code->constants);
  init_code(code);
}

int get_line(Code *code, int index) {
  int line_offset = 0;
  for (int i = 0; i < code->line_count; i++) {
    line_offset += code->instruction_counts[i];
    if (index < line_offset) {
      return code->lines[i]; // index is within this set of instructions; we're
                             // at this line
    }
  }
  return -1;
}
