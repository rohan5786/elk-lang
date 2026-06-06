#ifndef ELK_LANG_MEMORY_H
#define ELK_LANG_MEMORY_H

// least memory used lol
#define ADD_CAPACITY(capacity) ((capacity < 8) ? 8 : (capacity) * 2)
// gives the new sized array casted to the type it is (some amnt of bytes, uint8_t for example)
#define ADD_ARR(type, pointer, old_count, new_count) (type*) reallocate(pointer, sizeof(type) * (old_count), sizeof(type) * (new_count))

// size_t are in bytes, so we use sizeof to give it
void *reallocate(void *pointer, size_t old_size, size_t new_size);

#endif