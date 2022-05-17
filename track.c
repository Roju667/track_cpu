#include "signal.h"
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
#include "track.h"

static void *reader_task(void *argument);
static void *analyzer_task(void *argument);
static void *printer_task(void *argument);
static void *watchdog_task(void *argument);
static void *logger_task(void *argument);

static void write_new_log_msg(const char *new_log);
static void post_watchdog_sem(sem_t *sem);
static void trywait_watchdog_sem(sem_t *sem);
static void init_all_semaphores(void);
static void start_threads(void);
static void wait_threads_finished(void);
static void destroy_mutexes_semaphores(void);
static void cancel_threads(void);
static void init_sigterm_exit(void);

static my_thread_t threads[NO_THREADS] = {{0, logger_task, {{0}}, 0},
                                          {0, reader_task, {{0}}, 0},
                                          {0, analyzer_task, {{0}}, 0},
                                          {0, printer_task, {{0}}, 0},
                                          {0, watchdog_task, {{0}}, 0}};

static pthread_mutex_t muts[NUMBER_OF_MUTS] = {PTHREAD_MUTEX_INITIALIZER};
static sem_t sems[NUMBER_OF_SEMS];
static my_semaphore_t my_sems[NUMBER_OF_SEMS] = {
    {&sems[SEM_LOG_DATA], 0, 0},
    {&sems[SEM_RAW_DATA_POSTED], 0, 0},
    {&sems[SEM_WD_READER], 0, 1},
    {&sems[SEM_WD_ANALYZER], 0, 1},
    {&sems[SEM_WD_PRITNER], 0, 1}};

/* Shared data */
static ringbuffer_t ringbuffer;
static char log_msg[MAX_LOG_TEXT];
static char print_msg[MAX_PRINT_TEXT];
static volatile uint8_t watchdog_timeout = 0;
static volatile sig_atomic_t kill_process = 0;

int main()
{

  init_sigterm_exit();
  init_all_semaphores();
  start_threads();
  wait_threads_finished();
  destroy_mutexes_semaphores();

  return 0;
}

/* Read raw data from proc/stat file and push it to ringbuffer fifo */
static void *reader_task(void *argument)
{

  uint32_t text_len = 0;
  char raw_stats[MAX_MSG_LENGHT] = {0};
  UNUSED(argument);

  while (!kill_process && !watchdog_timeout)
    {
      text_len = get_raw_data(raw_stats);

      pthread_mutex_lock(&muts[MUT_RING_BUFF]);
      if (rb_is_enough_space(&ringbuffer, text_len))
        {
          rb_write_string(&ringbuffer, raw_stats, text_len);
        }
      else
        {
          write_new_log_msg("Warning : Ring buffer full\n");
        }
      sem_post(my_sems[SEM_RAW_DATA_POSTED].sem);
      pthread_mutex_unlock(&muts[MUT_RING_BUFF]);

      post_watchdog_sem(my_sems[SEM_WD_READER].sem);
      /* 100000 us = 0.1s = 100 jiffies */
      usleep(500000);
    }

  pthread_exit(0);
  return NULL;
}

/* Parse raw data,calculate, and send it to printer */
static void *analyzer_task(void *argument)
{

  cpu_t cpus[MAX_NO_CPUS] = {};
  char raw_stats[MAX_MSG_LENGHT] = {0};
  UNUSED(argument);

  while (!kill_process && !watchdog_timeout)
    {
      sem_wait(my_sems[SEM_RAW_DATA_POSTED].sem);

      pthread_mutex_lock(&muts[MUT_RING_BUFF]);
      rb_read_string(&ringbuffer, raw_stats);
      pthread_mutex_unlock(&muts[MUT_RING_BUFF]);

      pthread_mutex_lock(&muts[MUT_PRINT]);
      prepare_print(cpus, raw_stats, print_msg);
      pthread_mutex_unlock(&muts[MUT_PRINT]);

      post_watchdog_sem(my_sems[SEM_WD_ANALYZER].sem);
    }

  pthread_exit(0);
  return NULL;
}

/* Print cpu usage data in terminal */
static void *printer_task(void *argument)
{
  UNUSED(argument);

  while (!kill_process && !watchdog_timeout)
    {
      pthread_mutex_lock(&muts[MUT_PRINT]);
      system("clear");
      fprintf(stderr, "%s", print_msg);
      pthread_mutex_unlock(&muts[MUT_PRINT]);

      post_watchdog_sem(my_sems[SEM_WD_PRITNER].sem);

      sleep(PRINTER_TIME);
    }

  pthread_exit(0);
  return NULL;
}

/* Set termination flag if threads are delayed more than 2s */
static void *watchdog_task(void *argument)
{
  UNUSED(argument);
  while (1)
    {
      trywait_watchdog_sem(my_sems[SEM_WD_READER].sem);
      trywait_watchdog_sem(my_sems[SEM_WD_ANALYZER].sem);
      trywait_watchdog_sem(my_sems[SEM_WD_PRITNER].sem);
      if (watchdog_timeout || kill_process)
        {
          if (kill_process == 1)
            {
              perror("\nExit with SIGTERM \n");
            }
          cancel_threads();
          pthread_exit(0);
          break;
        }
      sleep(WATCHDOG_TIME);
    }
  return NULL;
}

/* Log debug data to textfile */
static void *logger_task(void *argument)
{
  /* Start new file */
  FILE *log_file = fopen("log_data.txt", "w");
  fclose(log_file);

  UNUSED(argument);

  while (!kill_process && !watchdog_timeout)
    {
      sem_wait(my_sems[SEM_LOG_DATA].sem);

      pthread_mutex_lock(&muts[MUT_LOG_DATA]);
      log_file = fopen("log_data.txt", "a");
      fputs(log_msg, log_file);
      fclose(log_file);
      pthread_mutex_unlock(&muts[MUT_LOG_DATA]);
    }

  pthread_exit(0);
  return NULL;
}

static void write_new_log_msg(const char *new_log)
{
  pthread_mutex_lock(&muts[MUT_LOG_DATA]);

  if (strlen(new_log) > MAX_LOG_TEXT)
    {
      strcpy(log_msg, "Error: Used log too long \n");
    }
  else
    {
      strcpy(log_msg, new_log);
    }

  sem_post(my_sems[SEM_LOG_DATA].sem);
  pthread_mutex_unlock(&muts[MUT_LOG_DATA]);

  return;
}

/* Binary semaphore post */
static void post_watchdog_sem(sem_t *sem)
{
  int32_t sem_value = 0;
  if (0 != sem_getvalue(sem, &sem_value))
    {
      write_new_log_msg("Error: Get semaphore val \n");
    }

  if (0 == sem_value)
    {
      sem_post(sem);
    }

  return;
}

static void trywait_watchdog_sem(sem_t *sem)
{

  if (-1 == sem_trywait(sem))
    {
      write_new_log_msg("Error: Watchdog timeout \n");
      watchdog_timeout = 1;
    }

  return;
}

static void init_all_semaphores(void)
{

  for (uint8_t i = 0; i < NUMBER_OF_SEMS; i++)
    {
      if (0 != sem_init(my_sems[i].sem, my_sems[i].pshared, my_sems[i].value))
        {
          write_new_log_msg("Error: Semaphore Init\n");
        }
    }

  return;
}

static void start_threads(void)
{
  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      pthread_attr_init(&threads[i].attr);
      if (0 !=
          pthread_create(&threads[i].id, NULL, threads[i].fun, &threads[i].arg))
        {
          write_new_log_msg("Error: Create task\n");
        }
      else
        {
          write_new_log_msg("Info: Task created\n");
        }
    }

  return;
}

static void wait_threads_finished(void)
{
  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      if (0 != pthread_join(threads[i].id, NULL))
        {
          write_new_log_msg("Error: Join thread\n");
        }
    }

  return;
}

static void destroy_mutexes_semaphores(void)
{

  for (uint32_t i = 0; i < NUMBER_OF_MUTS; i++)
    {
      if (0 != pthread_mutex_destroy(&muts[i]))
        {
          write_new_log_msg("Error: Destroy Mutex\n");
        }
    }

  for (uint8_t i = 0; i < NUMBER_OF_SEMS; i++)
    {
      if (0 != sem_destroy(my_sems[i].sem))
        {
          write_new_log_msg("Error: Destroy Semaphore\n");
        }
    }

  return;
}

static void cancel_threads(void)
{
  for (uint8_t i = 0; i < NO_THREADS; i++)
    {
      if (0 != pthread_cancel(threads[i].id))
        {
          write_new_log_msg("Error: Cancel Thread\n");
        }
    }

  return;
}

static void terminate_process(int signum)
{

  UNUSED(signum);
  kill_process = 1;
}

static void init_sigterm_exit(void)
{
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = terminate_process;
  sigaction(SIGTERM, &action, NULL);

  return;
}