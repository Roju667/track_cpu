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
#define MAX_LOG_TEXT 32U

typedef void *(*thread_function)(void *);

typedef struct
{
  pthread_t id;
  thread_function fun;
  pthread_attr_t attr;
  void *arg;
} my_thread_t;

static void *reader_task(void *argument);
static void *analyzer_task(void *argument);
static void *printer_task(void *argument);
static void *watchdog_task(void *argument);
static void *logger_task(void *argument);
static void write_new_log_msg(const char *new_log);

static pthread_mutex_t mmut_access_ring_buff = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mmut_access_cpus_stats = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mmut_access_log_msg = PTHREAD_MUTEX_INITIALIZER;

static sem_t ssem_log_data = {0}, ssem_raw_data_posted = {0};

static ringbuffer_t ringbuffer = {0};
static cpu_t cpus[MAX_NO_CPUS];
static cpu_t prev_cpus[MAX_NO_CPUS];
static uint32_t cpu_usage[MAX_NO_CPUS] = {0};
static uint32_t no_cups_used = 0;
static char log_msg[MAX_LOG_TEXT] = {0};

volatile uint32_t log_status = 0;

int main()
{
  my_thread_t threads[NO_THREADS] = {{0, logger_task, {{0}}, 0},
                                     {0, reader_task, {{0}}, 0},
                                     {0, analyzer_task, {{0}}, 0},
                                     {0, printer_task, {{0}}, 0},
                                     {0, watchdog_task, {{0}}, 0}};

  sem_init(&ssem_log_data, 0, 1);
  sem_init(&ssem_raw_data_posted, 0, 0);

  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      pthread_attr_init(&threads[i].attr);
      if (0 !=
          pthread_create(&threads[i].id, NULL, threads[i].fun, &threads[i].arg))
        {
          write_new_log_msg("Failed to create a thread");
        }

      write_new_log_msg("Task created\n");
    }

  /* Wait for threads to be finished */
  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      pthread_join(threads[i].id, NULL);
    }

  pthread_mutex_destroy(&mmut_access_ring_buff);
  pthread_mutex_destroy(&mmut_access_cpus_stats);
  pthread_mutex_destroy(&mmut_access_log_msg);

  while (1)
    {
    }

  return 0;
}

static void *reader_task(void *argument)
{

  uint32_t text_len = 0;
  char raw_stats[MAX_MSG_LENGHT] = {0};
  UNUSED(argument);
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
          write_new_log_msg("Data discarded\n");
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
      rb_read_string(&ringbuffer, raw_stats);
      pthread_mutex_unlock(&mmut_access_ring_buff);

      pthread_mutex_lock(&mmut_access_cpus_stats);
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
  /* Start new file */
  FILE *log_file = fopen("log_data.txt", "w");
  fclose(log_file);

  UNUSED(argument);

  while (1)
    {
      sem_wait(&ssem_log_data);

      pthread_mutex_lock(&mmut_access_log_msg);
      log_file = fopen("log_data.txt", "a");
      fputs(log_msg, log_file);
      fclose(log_file);
      pthread_mutex_unlock(&mmut_access_log_msg);
    }

  pthread_exit(0);
  return NULL;
}

static void write_new_log_msg(const char *new_log)
{
  pthread_mutex_lock(&mmut_access_log_msg);
  strcpy(log_msg, new_log);
  sem_post(&ssem_log_data);
  pthread_mutex_unlock(&mmut_access_log_msg);

  return;
}
