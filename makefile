# Set you compiler flag (gcc or clang)
CC := gcc


CORE_FLAGS :=  -std=c99 -Werror -g -D _DEFAULT_SOURCE
CORE_FLAGS_GCC := -Wall -Wextra -lpthread
CORE_FLAGS_CLANG := -Weverything

ifeq ($(CC),gcc)
	CORE_FLAGS += $(CORE_FLAGS_GCC)
endif

ifeq ($(CC),clang)
	CORE_FLAGS += $(CORE_FLAGS_CLANG)
endif

all:track

track: track.o manage_cpu_data.o ringbuffer.o
	$(CC) -o track track.o manage_cpu_data.o ringbuffer.o $(CORE_FLAGS)
	@echo "compilation with $(CC), edit CC variable to compile with clang"

track.o: track.c manage_cpu_data.h ringbuffer.h
	$(CC) -c track.c $(CORE_FLAGS)

manage_cpu_data.o: manage_cpu_data.c
	$(CC) -c manage_cpu_data.c $(CORE_FLAGS)

ringbuffer.o: ringbuffer.c
	$(CC) -c ringbuffer.c $(CORE_FLAGS)


clean:
	rm *.o *.exe *.txt *.gch

val:
	valgrind --leak-check=full ./track