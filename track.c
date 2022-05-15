#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#define MAX_MSG_LENGHT 2048U
#define MAX_NO_CPUS 16U
#define NO_CPU_PARAMS 10U
#define UNUSED(x) (void)(x)

typedef struct
{
  uint32_t user;
  uint32_t nice;
  uint32_t system;
  uint32_t idle;
  uint32_t iowait;
  uint32_t irq;
  uint32_t softirq;
  uint32_t steal;
  uint32_t guest;
  uint32_t guest_nice;
} cpu_usage_t;

typedef struct
{
  char name[8];
  cpu_usage_t usage;
} cpu_t;

static void get_raw_data(char *destination);
static uint32_t parse_text_to_struct(char *text_from_file, cpu_t *cpus);
static uint32_t calculate_cpu_usage(const cpu_t *cpu, const cpu_t *prev_cpu);

int main()
{
  char raw_stats[MAX_MSG_LENGHT];
  cpu_t cpus[16];
  cpu_t prev_cpus[16];
  uint32_t cpu_usage[16] = {0};
  uint32_t no_cups_used = 0;

  while (1)
    {
      get_raw_data(raw_stats);
      parse_text_to_struct(raw_stats, prev_cpus);

      sleep(1);
      get_raw_data(raw_stats);
      no_cups_used = parse_text_to_struct(raw_stats, cpus);

      system("clear");
      for (uint8_t i = 0; i < no_cups_used; i++)
        {
          cpu_usage[i] = calculate_cpu_usage(&cpus[i], &prev_cpus[i]);
          fprintf(stderr, "%s: %d %% \n", cpus[i].name, cpu_usage[i]);
        }
      sleep(1);
    }

  return 0;
}

static void get_raw_data(char *destination)
{
  FILE *fp = fopen("/proc/stat", "r");

  char c;
  uint32_t len = 0;

  if (NULL == fp)
    {
      printf("erorr\n");
      perror("Error opning /proc/stat");
    }
  else
    {
      while (EOF != (c = fgetc(fp)) && ++len < MAX_MSG_LENGHT)
        {
          destination[len] = c;
        }
    }

  fclose(fp);
  return;
}

static bool is_this_cpu_data(const char *cpu_name)
{
  bool ret_val = false;

  if (NULL != cpu_name)
    {
      if (0 == strncmp("cpu", cpu_name, 3))
        {
          ret_val = true;
        }
    }

  return ret_val;
}

static uint32_t parse_text_to_struct(char *text_from_file, cpu_t *cpus)
{

  char *cpu_name = NULL;
  char *cpu_param_txt = NULL;
  uint32_t *cpu_val_ptr = NULL;
  uint8_t no_cpus = 0;
  uint32_t find_first_cpu = 0;

  while (text_from_file[find_first_cpu] != 'c')
    {
      find_first_cpu++;
    }

  cpu_name = strtok(&text_from_file[find_first_cpu], " ");
  for (no_cpus = 0; no_cpus < MAX_NO_CPUS; no_cpus++)
    {
      if (false == is_this_cpu_data(cpu_name))
        {
          break;
        }
      strcpy(cpus[no_cpus].name, cpu_name);

      cpu_val_ptr = (uint32_t *)&(cpus[no_cpus].usage);
      for (uint8_t j = 0; j < NO_CPU_PARAMS - 1; j++)
        {
          cpu_param_txt = strtok(NULL, " ");
          *cpu_val_ptr = atoi(cpu_param_txt);
          cpu_val_ptr++;
        }

      strtok(NULL, "\n");
      cpu_name = strtok(NULL, " ");
    }

  return no_cpus;
}

static uint32_t calculate_cpu_usage(const cpu_t *cpu, const cpu_t *prev_cpu)
{

  uint32_t prev_idle = 0;
  uint32_t prev_non_idle = 0;
  uint32_t idle = 0;
  uint32_t non_idle = 0;
  uint32_t totald = 0;
  uint32_t idled = 0;
  uint32_t cpu_precetage = 0;

  prev_idle = prev_cpu->usage.idle + prev_cpu->usage.iowait;
  idle = cpu->usage.idle + cpu->usage.iowait;

  prev_non_idle = prev_cpu->usage.user + prev_cpu->usage.nice +
                  prev_cpu->usage.system + prev_cpu->usage.irq +
                  prev_cpu->usage.softirq + prev_cpu->usage.steal;

  non_idle = cpu->usage.user + cpu->usage.nice + cpu->usage.system +
             cpu->usage.irq + cpu->usage.softirq + cpu->usage.steal;

  /* total amount of ticks between measurments */
  totald = (idle + non_idle) - (prev_idle + prev_non_idle);
  /* idle amount of ticks between measurements */
  idled = idle - prev_idle;

  if (totald != 0)
    {
      cpu_precetage = (100 * (totald - idled)) / totald;
    }

  return cpu_precetage;
}