SIMGRID_PATH = /usr/local
CC = gcc
LIBS := -lsimgrid -lm

SOURCES = \
src/dax.c \
src/task.c \
src/host.c \
src/scheduling.c \
src/dpds.c \
src/main.c 

OBJS = \
src/dax.o \
src/task.o \
src/host.o \
src/scheduling.o \
src/dpds.o \
src/main.o

all: EnsembleSched

EnsembleSched: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L$(SIMGRID_PATH)/lib -o EnsembleSched $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

%.o: %.c
	$(CC)  -I$(SIMGRID_PATH)/include -I"./include" -O3 -Wall -c -o $@ $<


# Other Targets
clean:
	rm -rf $(OBJS) EnsembleSched

