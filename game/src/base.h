#ifndef BASE_H
#define BASE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;
typedef int32_t bool32;

typedef float  float32;
typedef double float64;

#define kilobytes(value) ((value) * 1024ULL)
#define megabytes(value) ((value) * 1024ULL * 1024ULL)
#define gigabytes(value) ((value) * 1024ULL * 1024ULL * 1024ULL)
#define terabytes(value) ((value) * 1024ULL * 1024ULL * 1024ULL * 1024ULL)

// It's kinda like std::vector...
//
// I was thinking of using an arena instead since I have been enjoying using them
// in other projects, but since the scale of this game is so small, using these
// dynamic arrays which reallocates and moves the memory around isn't too bad.
template <typename T>
struct Auto_Array {
	T *data;
	size_t count;
	size_t capacity;
};

template <typename T>
void array_init(Auto_Array<T> *array, size_t initial_capacity = 8) {
	assert(array);
	array->data     = (T *)malloc(sizeof(T) * initial_capacity);
	array->count    = 0;
	array->capacity = initial_capacity;
	assert(array->data);
}

template <typename T>
void array_add(Auto_Array<T> *array, T value) {
	assert(array);
	if (array->count >= array->capacity) {
		size_t new_capacity = array->capacity * 2;
		array->data = (T *)realloc(array->data, sizeof(T) * new_capacity);
		assert(array->data);
		array->capacity = new_capacity;
	}
	array->data[array->count++] = value;
}

template <typename T>
T *array_get_at_index(Auto_Array<T> *array, size_t index) {
	assert(index < array->count);
	return &array->data[index];
}

template <typename T>
void array_unordered_remove(Auto_Array<T> *array, size_t index) {
	assert(array);
	assert(index < array->count);

	array->data[index] = array->data[array->count - 1];
	array->count--;
}

template <typename T>
void array_ordered_remove(Auto_Array<T> *array, size_t index) {
	assert(array);
	assert(index < array->count);

	size_t num_of_elements_to_shift = array->count - index - 1;
	if (num_of_elements_to_shift > 0) {
		memmove(&array->data[index], &array->data[index + 1], num_of_elements_to_shift * sizeof(T));
	}
	array->count--;
}

template <typename T>
void array_free(Auto_Array<T> *array) {
	assert(array);
	free(array->data);
	
	array->data     = nullptr;
	array->count    = 0;
	array->capacity = 0;
}

#define For(array) \
	for (auto it = (array).data; it < (array).data + (array).count; ++it)

#endif
