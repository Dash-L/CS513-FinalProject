#pragma once
#include "vec.h"

// Threadsafe queue
#define QUEUE(T)                                                               \
  typedef struct {                                                             \
    T##_VEC *vec;                                                              \
    pthread_mutex_t mutex;                                                     \
    pthread_cond_t nonempty;                                                   \
  } T##_QUEUE;                                                                 \
                                                                               \
  T##_QUEUE *T##_QUEUE_create() {                                              \
    void T##_QUEUE_push(T##_QUEUE *queue, T elem);                             \
    /* Guaranteed to pop an element, might wait a while for one to be there */ \
    void T##_QUEUE_pop(T##_QUEUE *queue, T *out);                              \
    /* Returns -1 if there is nothing to pop. Still waits on mutex until it    \
     * unlocks */                                                              \
    int T##_QUEUE_trypop(T##_QUEUE *queue, T *out);

#define QUEUE_IMPL(T)                                                          \
  T##_QUEUE *T##_QUEUE_create() {                                              \
    T##_QUEUE *queue = (T##_QUEUE *)malloc(sizeof(*queue));                    \
    T##_VEC *vec = T##_VEC_create();                                           \
    queue->vec = vec;                                                          \
    pthread_mutex_init(&queue->mutex, NULL);                                   \
    pthread_cond_init(&queue->nonempty, NULL);                                 \
    return queue;                                                              \
  }                                                                            \
  void T##_QUEUE_push(T##_QUEUE *queue, T elem) {                              \
    pthread_mutex_lock(&queue->mutex);                                         \
    T##_VEC_append(queue->vec, elem);                                          \
    pthread_cond_signal(&queue->nonempty);                                     \
    pthread_mutex_unlock(&queue->mutex);                                       \
  }                                                                            \
  /* Guaranteed to pop an element, might wait a while for one to be there */   \
  void T##_QUEUE_pop(T##_QUEUE *queue, T *out) {                               \
    pthread_mutex_lock(&queue->mutex);                                         \
    while (queue->vec->size == 0) {                                            \
      pthread_cond_wait(&queue->nonempty, &queue->mutex);                      \
    }                                                                          \
    *out = T##_VEC_pop(queue->vec);                                            \
    pthread_mutex_unlock(&queue->mutex);                                       \
  }                                                                            \
  /* Returns -1 if there is nothing to pop. Still waits on mutex until it      \
   * unlocks */                                                                \
  int T##_QUEUE_trypop(T##_QUEUE *queue, T *out) {                             \
    if (queue->vec->size == 0)                                                 \
      return -1;                                                               \
    pthread_mutex_lock(&queue->mutex);                                         \
    if (queue->vec->size == 0) {                                               \
      pthread_mutex_unlock(&queue->mutex);                                     \
      return -1;                                                               \
    }                                                                          \
    *out = T##_VEC_pop(queue->vec);                                            \
    pthread_mutex_unlock(&queue->mutex);                                       \
    return 0;                                                                  \
  }
