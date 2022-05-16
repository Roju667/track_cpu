#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "pthread.h"
#include "semaphore.h"

#include "manage_cpu_data.h"
#include "ringbuffer.h"

#define UNUSED(x) (void)(x)
#define NO_THREADS 5U

static void *reader_task(void *argument);
static void *analyzer_task(void *argument);
static void *printer_task(void *argument);
static void *watchdog_task(void *argument);
static void *logger_task(void *argument);

pthread_mutex_t mmut_access_ring_buff = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mmut_access_cpus_stats = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mmut_3 = PTHREAD_MUTEX_INITIALIZER;

sem_t ssem_analzyer_ready, ssem_raw_data_posted;

typedef void *(*thread_function)(void *);

typedef struct
{
  pthread_t id;
  thread_function fun;
  pthread_attr_t attr;
  void *arg;
} my_thread_t;

ringbuffer_t ringbuffer = {0};
cpu_t cpus[MAX_NO_CPUS];
cpu_t prev_cpus[MAX_NO_CPUS];
uint32_t cpu_usage[MAX_NO_CPUS] = {0};
uint32_t no_cups_used = 0;

int main()
{
  my_thread_t threads[NO_THREADS] = {{0, reader_task, {{0}}, 0},
                                     {0, analyzer_task, {{0}}, 0},
                                     {0, printer_task, {{0}}, 0},
                                     {0, watchdog_task, {{0}}, 0},
                                     {0, logger_task, {{0}}, 0}};

  sem_init(&ssem_analzyer_ready, 0, 1);
  sem_init(&ssem_raw_data_posted, 0, 0);

  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      pthread_attr_init(&threads[i].attr);
      if (0 !=
          pthread_create(&threads[i].id, NULL, threads[i].fun, &threads[i].arg))
        {
          perror("Failed to create a thread");
        }
    }

  /* Wait for threads to be finished */
  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      pthread_join(threads[i].id, NULL);
    }

  pthread_mutex_destroy(&mmut_access_ring_buff);
  pthread_mutex_destroy(&mmut_access_cpus_stats);
  pthread_mutex_destroy(&mmut_3);

  while (1)
    {
    }

  return 0;
}

static void *reader_task(void *argument)
{
  UNUSED(argument);
  uint32_t text_len = 0;
  char raw_stats[MAX_MSG_LENGHT] = {0};
  while (1)
    {
      text_len = get_raw_data(raw_stats);
      pthread_mutex_lock(&mmut_access_ring_buff);

      if (rb_is_enough_space(&ringbuffer, text_len))
        {
          rb_write_string(&ringbuffer, raw_stats, text_len);
        }
      else
        {
          /* log data discarded */
        }
      sem_post(&ssem_raw_data_posted);
      pthread_mutex_unlock(&mmut_access_ring_buff);
      /* 100000 us = 0.1s = 100 jiffies */
      usleep(500000);
    }

  pthread_exit(0);
  return NULL;
}

static void *analyzer_task(void *argument)
{
  UNUSED(argument);
  char raw_stats[MAX_MSG_LENGHT] = {0};

  while (1)
    {
      sem_wait(&ssem_raw_data_posted);
      pthread_mutex_lock(&mmut_access_ring_buff);
      pthread_mutex_lock(&mmut_access_cpus_stats);
      rb_read_string(&ringbuffer, raw_stats);
      pthread_mutex_unlock(&mmut_access_ring_buff);
      memcpy(prev_cpus, cpus, sizeof(cpu_t) * MAX_NO_CPUS);
      no_cups_used = parse_text_to_struct(raw_stats, cpus);
      pthread_mutex_unlock(&mmut_access_cpus_stats);
    }

  pthread_exit(0);
  return NULL;
}

static void *printer_task(void *argument)
{
  UNUSED(argument);

  while (1)
    {
      pthread_mutex_lock(&mmut_access_cpus_stats);
      system("clear");
      for (uint8_t i = 0; i < no_cups_used; i++)
        {
          cpu_usage[i] = calculate_cpu_usage(&cpus[i], &prev_cpus[i]);
          fprintf(stderr, "%s: %d %% \n", cpus[i].name, cpu_usage[i]);
        }
      pthread_mutex_unlock(&mmut_access_cpus_stats);
      sleep(1);
    }
  pthread_exit(0);
  return NULL;
}

static void *watchdog_task(void *argument)
{
  UNUSED(argument);
  pthread_exit(0);
  return NULL;
}

static void *logger_task(void *argument)
{
  UNUSED(argument);
  pthread_exit(0);
  return NULL;
}