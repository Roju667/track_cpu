#include "stdbool.h"
#include "stdint.h"
#include "ringbuffer.h"
#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"

rb_status rb_read(ringbuffer_t *buffer, uint8_t *value)
{
  if (buffer->head == buffer->tail)
    {
      return RB_ERROR;
    }

  *value = buffer->buffer[buffer->tail];

  buffer->tail = (buffer->tail + 1) % RING_BUFFER_SIZE;

  return RB_OK;
}

rb_status rb_write(ringbuffer_t *buffer, uint8_t value)
{

  uint16_t HeadTmp;
  HeadTmp = (buffer->head + 1) % RING_BUFFER_SIZE;

  if (HeadTmp == buffer->tail)
    {
      return RB_ERROR;
    }

  buffer->buffer[buffer->head] = value;
  buffer->head = HeadTmp;

  return RB_OK;
}

void rb_flush(ringbuffer_t *buffer)
{
  buffer->head = 0;
  buffer->tail = 0;
}

bool rb_is_enough_space(const ringbuffer_t *buffer, uint32_t msg_lenght)
{
  uint32_t remaining = 0;
  if (buffer->tail > buffer->head)
    {
      remaining = buffer->tail - buffer->head;
    }
  else
    {
      remaining = RING_BUFFER_SIZE - buffer->head + buffer->tail;
    }
  return (remaining > msg_lenght);
}

rb_status rb_write_string(ringbuffer_t *buffer, const char *msg, uint32_t len)
{
  uint32_t i = len;
  rb_status ret_status = RB_ERROR;

  for (i = 0; i < len; i++)
    {
      ret_status = rb_write(buffer, msg[i]);
    }

  rb_write(buffer, MSG_DONE_CHAR);

  return ret_status;
}

rb_status rb_read_string(ringbuffer_t *buffer, char *destination)
{
  uint32_t i = 0;
  rb_status status;

  while (1)
    {
      status = rb_read(buffer, (uint8_t *)&destination[i]);

      if (MSG_DONE_CHAR == destination[i] || RB_ERROR == status)
        {
          break;
        }

      i++;
    }
  return status;
}