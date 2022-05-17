#ifndef INC_RINGBUFFER_H_
#define INC_RINGBUFFER_H_

#include "stdint.h"

#define RING_BUFFER_SIZE 4096
#define MSG_DONE_CHAR 4

typedef struct
{
  char buffer[RING_BUFFER_SIZE];
  uint32_t head;
  uint32_t tail;
} ringbuffer_t;

typedef enum
{
  RB_OK = 0,
  RB_ERROR = -1
} rb_status;

rb_status rb_write(ringbuffer_t *buffer, char value);
rb_status rb_read(ringbuffer_t *buffer, char *value);
void rb_flush(ringbuffer_t *buffer);
bool rb_is_enough_space(const ringbuffer_t *buffer, uint32_t msg_lenght);
rb_status rb_write_string(ringbuffer_t *buffer, const char *msg, uint32_t len);
rb_status rb_read_string(ringbuffer_t *buffer, char *destination);

#endif /* INC_RINGBUFFER_H_ */
