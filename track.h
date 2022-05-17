#ifndef INC_TRACK_H_
#define INC_TRACK_H_

#include "pthread.h"
#include "semaphore.h"
#include "stdint.h"

#define UNUSED(x) (void)(x)
#define NO_THREADS 5U
#define NO_MUTEXES 3U
#define MAX_LOG_TEXT 32U
#define WATCHDOG_TIME 2U
#define PRINTER_TIME 1U

typedef void *(*thread_function)(void *);

typedef struct
{
  pthread_t id;
  thread_function fun;
  pthread_attr_t attr;
  void *arg;
} my_thread_t;

enum semaphores
{
  SEM_LOG_DATA = 0,
  SEM_RAW_DATA_POSTED = 1,
  SEM_WD_READER = 2,
  SEM_WD_ANALYZER = 3,
  SEM_WD_PRITNER = 4,
  NUMBER_OF_SEMS
};

typedef struct
{
  sem_t *sem;
  int32_t pshared;
  uint32_t value;
} my_semaphore_t;

enum mutexes
{
  MUT_RING_BUFF = 0,
  MUT_PRINT = 1,
  MUT_LOG_DATA = 2,
  NUMBER_OF_MUTS
};

#endif /*  INC_TRACK_H_  */
