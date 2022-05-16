# Set you compiler flag (gcc or clang)
CC := gcc


CORE_FLAGS := -lpthread -std=c99 -Werror -ggdb
CORE_FLAGS_GCC := -Wall -Wextra
CORE_FLAGS_CLANG := -Weverything

ifeq ($(CC),gcc)
	CORE_FLAGS += $(CORE_FLAGS_GCC)
endif

ifeq ($(CC),clang)
	CORE_FLAGS += $(CORE_FLAGS_CLANG)
endif

all:track

track: track.o manage_cpu_data.o
	$(CC) -o track track.o manage_cpu_data.o $(CORE_FLAGS)
	@echo "compilation with $(CC), edit CC variable to compile with clang"

track.o: track.c manage_cpu_data.h
	$(CC) -c track.c $(CORE_FLAGS)

manage_cpu_data.o: manage_cpu_data.c manage_cpu_data.h
	$(CC) -c manage_cpu_data.c manage_cpu_data.h $(CORE_FLAGS)


clean:
	rm *.o *.exe *.txt

val:
	valgrind --leak-check-full ./track