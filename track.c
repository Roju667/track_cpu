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
static void *logger_task(void *argument);

pthread_mutex_t mmut_access_raw_data = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mmut_access_cpus_stats = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mmut_3 = PTHREAD_MUTEX_INITIALIZER;

sem_t ssem_analzyer_ready, ssem_reader_ready;

typedef void *(*thread_function)(void *);

typedef struct
{
  pthread_t id;
  thread_function fun;
  pthread_attr_t attr;
  void *arg;
} my_thread_t;

char raw_stats[MAX_MSG_LENGHT] = {0};
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

  // pthread_t thread_id[NO_THREADS] = {0};
  // thread_function thread_fun[NO_THREADS] = {
  //     reader_task, analyzer_task, printer_task, watchdog_task, logger_task};
  // pthread_attr_t attr[NO_THREADS] = {0};
  // uint32_t arg[NO_THREADS] = {0};

  sem_init(&ssem_analzyer_ready, 0, 1);
  sem_init(&ssem_reader_ready, 0, 0);

  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      pthread_attr_init(&threads[i].attr);
      if (0 !=
          pthread_create(&threads[i].id, NULL, threads[i].fun, &threads[i].arg))
        {
          perror("Failed to create a thread");
        }
    }

  // for (uint8_t i = 0; i < NO_THREADS; i++)
  //   {
  //     pthread_attr_init(&attr[i]);
  //     arg[i] = i;
  //     if (0 != pthread_create(&thread_id[i], NULL, thread_fun[i], &arg[i]))
  //       {
  //         perror("Failed to create a thread");
  //       }
  //   }

  /* Wait for threads to be finished */
  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      pthread_join(threads[i].id, NULL);
    }

  pthread_mutex_destroy(&mmut_access_raw_data);
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

  while (1)
    {
      sem_wait(&ssem_analzyer_ready);
      pthread_mutex_lock(&mmut_access_raw_data);
      get_raw_data(raw_stats);
      sem_post(&ssem_reader_ready);
      pthread_mutex_unlock(&mmut_access_raw_data);
      sleep(1);
    }

  pthread_exit(0);
  return NULL;
}

static void *analyzer_task(void *argument)
{
  UNUSED(argument);

  while (1)
    {
      sem_wait(&ssem_reader_ready);
      pthread_mutex_lock(&mmut_access_raw_data);
      pthread_mutex_lock(&mmut_access_cpus_stats);
      memcpy(prev_cpus, cpus, sizeof(cpu_t) * MAX_NO_CPUS);
      no_cups_used = parse_text_to_struct(raw_stats, cpus);
      sem_post(&ssem_analzyer_ready);
      pthread_mutex_unlock(&mmut_access_cpus_stats);
      pthread_mutex_unlock(&mmut_access_raw_data);
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