#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#include "manage_cpu_data.h"

#define UNUSED(x) (void)(x)

char raw_stats[MAX_MSG_LENGHT];

int main()
{

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
