#pragma once
#include <stdlib.h>
#include <string.h>

#define VEC(T)                                                                 \
  typedef struct {                                                             \
    T *data;                                                                   \
    size_t size;                                                               \
    size_t cap;                                                                \
  } T##_VEC;                                                                   \
  T##_VEC *T##_VEC_create();                                                   \
  void T##_VEC_append(T##_VEC *vec, T elem);                                   \
  void T##_VEC_remove(T##_VEC *vec, size_t idx);                               \
  T T##_VEC_pop(T##_VEC *vec);

#define VEC_IMPL(T)                                                            \
  T##_VEC *T##_VEC_create() {                                                  \
    T##_VEC *vec = (T##_VEC *)malloc(sizeof(*vec));                            \
    vec->cap = 8;                                                              \
    vec->data = (T *)malloc(sizeof(T) * vec->cap);                             \
    vec->size = 0;                                                             \
    return vec;                                                                \
  }                                                                            \
  void T##_VEC_append(T##_VEC *vec, T elem) {                                  \
    if (vec->size >= vec->cap) {                                               \
      vec->cap *= 2;                                                           \
      vec->data = (T *)realloc(vec->data, sizeof(T) * vec->cap);               \
    }                                                                          \
    vec->data[vec->size++] = elem;                                             \
  }                                                                            \
  void T##_VEC_remove(T##_VEC *vec, size_t idx) {                              \
    memmove(vec->data + idx, vec->data + idx + 1,                              \
            (vec->size - idx - 1) * sizeof(T));                                \
    vec->size -= 1;                                                            \
  }                                                                            \
  T T##_VEC_pop(T##_VEC *vec) { return vec->data[--vec->size]; }
