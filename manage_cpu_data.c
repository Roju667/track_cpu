#include "stdint.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"

#include "manage_cpu_data.h"

void get_raw_data(char *destination)
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

uint32_t parse_text_to_struct(char *text_from_file, cpu_t *cpus)
{

  uint32_t no_cpus = 0;
  uint32_t args;
  char *text_line = text_from_file;

  if (text_line == NULL)
    {
      return 0;
    }

  text_line++;

  do
    {

      args = sscanf(text_line, "%s %u %u %u %u %u %u %u %u %u %u",
                    cpus[no_cpus].name, &cpus[no_cpus].usage.user,
                    &cpus[no_cpus].usage.nice, &cpus[no_cpus].usage.system,
                    &cpus[no_cpus].usage.idle, &cpus[no_cpus].usage.iowait,
                    &cpus[no_cpus].usage.irq, &cpus[no_cpus].usage.softirq,
                    &cpus[no_cpus].usage.steal, &cpus[no_cpus].usage.guest,
                    &cpus[no_cpus].usage.guest_nice);
      (void)args;
      // fprintf(stderr, "args written %d\n", args);
      // fprintf(stderr, "%s %u %u %u %u %u %u %u %u %u %u \n",
      // cpus[no_cpus].name,
      //         cpus[no_cpus].usage.user, cpus[no_cpus].usage.nice,
      //         cpus[no_cpus].usage.system, cpus[no_cpus].usage.idle,
      //         cpus[no_cpus].usage.iowait, cpus[no_cpus].usage.irq,
      //         cpus[no_cpus].usage.softirq, cpus[no_cpus].usage.steal,
      //         cpus[no_cpus].usage.guest, cpus[no_cpus].usage.guest_nice);

      if (false == is_this_cpu_data(cpus[no_cpus].name))
        {
          break;
        }

      if (0 == no_cpus)
        {
          text_line = strtok(text_line, "\n");
          text_line = strtok(NULL, "\n");
        }
      else
        {
          text_line = strtok(NULL, "\n");
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