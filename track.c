#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "pthread.h"
#include "semaphore.h"

#include "manage_cpu_data.h"

#define UNUSED(x) (void)(x)
#define NO_THREADS 5U

static void *reader_task(void *argument);
static void *analyzer_task(void *argument);
static void *printer_task(void *argument);
static void *watchdog_task(void *argument);
static void *debug_task(void *argument);

pthread_mutex_t access_raw_data_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t access_cpus_stats_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;

sem_t sem_analzyer_ready, sem_reader_ready;

typedef void *(*thread_function)(void *);

char raw_stats[MAX_MSG_LENGHT] = {0};
cpu_t cpus[MAX_NO_CPUS];
cpu_t prev_cpus[MAX_NO_CPUS];
uint32_t cpu_usage[MAX_NO_CPUS] = {0};
uint32_t no_cups_used = 0;

int main()
{
  sem_init(&sem_analzyer_ready, 0, 1);
  sem_init(&sem_reader_ready, 0, 0);

  pthread_t thread_id[NO_THREADS] = {0};
  thread_function thread_fun[NO_THREADS] = {
      reader_task, analyzer_task, printer_task, watchdog_task, debug_task};

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  UNUSED(thread_fun);
  uint32_t arg[NO_THREADS] = {0};
  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      arg[i] = i;
      if (0 != pthread_create(&thread_id[i], NULL, thread_fun[i], &arg[i]))
        {
          perror("Failed to create a thread");
        }
    }

  /* Wait for threads to be finished */
  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      pthread_join(thread_id[i], NULL);
    }

  pthread_mutex_destroy(&access_raw_data_mut);
  pthread_mutex_destroy(&access_cpus_stats_mut);
  pthread_mutex_destroy(&mutex_3);

  while (1)
    {
    }

  return 0;
}

static void *reader_task(void *argument)
{
  UNUSED(argument);

  while (1)
    {
      sem_wait(&sem_analzyer_ready);
      pthread_mutex_lock(&access_raw_data_mut);
      get_raw_data(raw_stats);
      sem_post(&sem_reader_ready);
      pthread_mutex_unlock(&access_raw_data_mut);
      sleep(1);
    }

  pthread_exit(0);
}

static void *analyzer_task(void *argument)
{
  UNUSED(argument);

  while (1)
    {
      sem_wait(&sem_reader_ready);
      pthread_mutex_lock(&access_raw_data_mut);
      pthread_mutex_lock(&access_cpus_stats_mut);
      memcpy(prev_cpus, cpus, sizeof(cpu_t) * MAX_NO_CPUS);
      no_cups_used = parse_text_to_struct(raw_stats, cpus);
      sem_post(&sem_analzyer_ready);
      pthread_mutex_unlock(&access_cpus_stats_mut);
      pthread_mutex_unlock(&access_raw_data_mut);
    }

  pthread_exit(0);
}

static void *printer_task(void *argument)
{
  UNUSED(argument);

  while (1)
    {
      pthread_mutex_lock(&access_cpus_stats_mut);
      system("clear");
      for (uint8_t i = 0; i < no_cups_used; i++)
        {
          cpu_usage[i] = calculate_cpu_usage(&cpus[i], &prev_cpus[i]);
          fprintf(stderr, "%s: %d %% \n", cpus[i].name, cpu_usage[i]);
        }
      pthread_mutex_unlock(&access_cpus_stats_mut);
      sleep(1);
    }
  pthread_exit(0);
}

static void *watchdog_task(void *argument)
{
  UNUSED(argument);
  pthread_exit(NULL);
}

static void *debug_task(void *argument)
{
  UNUSED(argument);
  pthread_exit(NULL);
}