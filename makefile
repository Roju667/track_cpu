# Set you compiler flag (gcc or clang)
COMPILER = ${CC}

CORE_FLAGS :=  -std=c99 -Werror -g -D _DEFAULT_SOURCE
CORE_FLAGS_GCC := -Wall -Wextra -lpthread
CORE_FLAGS_CLANG := -Weverything -Wno-disabled-macro-expansion

ifeq ($(COMPILER),gcc)
	CORE_FLAGS += $(CORE_FLAGS_GCC)
endif

ifeq ($(COMPILER),clang)
	CORE_FLAGS += $(CORE_FLAGS_CLANG)
endif

all:track

track: track.o manage_cpu_data.o ringbuffer.o
	$(COMPILER) -o track track.o manage_cpu_data.o ringbuffer.o $(CORE_FLAGS)

track.o: track.c manage_cpu_data.h ringbuffer.h track.h
	$(COMPILER) -c track.c $(CORE_FLAGS)

manage_cpu_data.o: manage_cpu_data.c
	$(COMPILER) -c manage_cpu_data.c $(CORE_FLAGS)

ringbuffer.o: ringbuffer.c
	$(COMPILER) -c ringbuffer.c $(CORE_FLAGS)


clean:
	rm *.o *.exe *.gch

val:
	valgrind --leak-check=full --track-origins=yes --log-file="valgrind_log.txt" ./track
