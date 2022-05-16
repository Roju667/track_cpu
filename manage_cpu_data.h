#ifndef INC_MANAGE_CPU_H_
#define INC_MANAGE_CPU_H_

#include "stdint.h"

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

#define MAX_MSG_LENGHT 2048U
#define MAX_NO_CPUS 16U
#define NO_CPU_PARAMS 10U

uint32_t get_raw_data(char *destination);
uint32_t parse_text_to_struct(char *text_from_file, cpu_t *cpus);
uint32_t calculate_cpu_usage(const cpu_t *cpu, const cpu_t *prev_cpu);

#endif /* INC_MANAGE_CPU_H_*/
