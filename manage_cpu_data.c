#include "stdint.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"

#include "manage_cpu_data.h"

#define REQUIRED_ARGS NO_CPU_PARAMS + 1

uint32_t get_raw_data(char *destination)
{
  FILE *fp = fopen("/proc/stat", "r");

  uint32_t len = 0;

  if (NULL == fp)
    {
      printf("erorr\n");
      perror("Error opning /proc/stat");
      return 0;
    }
  else
    {
      char c;
      while (EOF != (c = (char)fgetc(fp)) && ++len < MAX_MSG_LENGHT)
        {
          destination[len] = c;
        }
    }

  fclose(fp);
  return len;
}

static bool is_this_cpu_data(const char *cpu_name)
{
  bool is_cpu = false;

  if (NULL != cpu_name)
    {
      if (0 == strncmp("cpu", cpu_name, 3))
        {
          is_cpu = true;
        }
    }

  return is_cpu;
}

uint32_t parse_text_to_struct(char *text_from_file, cpu_t *cpus)
{

  uint32_t no_cpus = 0;
  int32_t args_scanned = 0;
  char *text_ptr = text_from_file;

  if (text_ptr == NULL)
    {
      return 0;
    }

  text_ptr++;

  do
    {

      args_scanned = sscanf(
          text_ptr, "%8s %u %u %u %u %u %u %u %u %u %u", cpus[no_cpus].name,
          &cpus[no_cpus].usage.user, &cpus[no_cpus].usage.nice,
          &cpus[no_cpus].usage.system, &cpus[no_cpus].usage.idle,
          &cpus[no_cpus].usage.iowait, &cpus[no_cpus].usage.irq,
          &cpus[no_cpus].usage.softirq, &cpus[no_cpus].usage.steal,
          &cpus[no_cpus].usage.guest, &cpus[no_cpus].usage.guest_nice);

      if ((true == is_this_cpu_data(cpus[no_cpus].name)) &
          (args_scanned != REQUIRED_ARGS))
        {
          /* logger msg - not enough cpu parameters */
        }

      if (false == is_this_cpu_data(cpus[no_cpus].name))
        {
          break;
        }

      if (0 == no_cpus)
        {
          text_ptr = strtok(text_ptr, "\n"); /* ? */
          text_ptr = strtok(NULL, "\n");
        }
      else
        {
          text_ptr = strtok(NULL, "\n");
        }

      no_cpus++;
    }
  while (1);
  return no_cpus;
}

uint32_t calculate_cpu_usage(const cpu_t *cpu, const cpu_t *prev_cpu)
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

void prepare_print(cpu_t *cpus, char *raw_stats, char *data_to_print)
{
  cpu_t prev_cpus[MAX_NO_CPUS] = {0};
  uint32_t no_cpus_used = 0;
  uint32_t cpu_usage[MAX_NO_CPUS] = {0};
  uint32_t offset = 0;

  memcpy(prev_cpus, cpus, sizeof(cpu_t) * MAX_NO_CPUS);
  memset(data_to_print, 0, MAX_PRINT_TEXT);

  no_cpus_used = parse_text_to_struct(raw_stats, cpus);
  for (uint8_t i = 0; i < no_cpus_used; i++)
    {
      cpu_usage[i] = calculate_cpu_usage(&cpus[i], &prev_cpus[i]);
      sprintf(data_to_print + offset, "%s: %d %% \n", cpus[i].name,
              cpu_usage[i]);

      offset = (uint32_t)strlen(data_to_print);
    }

  return;
}
