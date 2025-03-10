PNAME=fu
EXEC=$(PNAME).out

BIN=compiled/
CC=cc

SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.o,$(BIN)%.o,$(SOURCES:.c=.o))

INCLUDES=includes/
CFLAGS=-Wall -Wextra -I$(INCLUDES)

ifeq ($(BUILD),1)
CFLAGS += -O3 -DPROD
endif

ifeq ($(DEBUG), 1)
CFLAGS += -ggdb
endif

ifeq ($(DEBUG_TRACE_EXECUTION), 1)
CFLAGS += -DDEBUG_TRACE_EXECUTION
endif

ifeq ($(LOG), 1)
CFLAGS += -DLOG
endif 

ifeq ($(W64), 1)
CC = x86_64-w64-mingw32-gcc
EXEC = $(PNAME).exe
endif 

# Default target
all: $(EXEC)
	rm -f $(BIN)*.o

# Link the executable
$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) -o $(BIN)$(EXEC) 

# Compile source files into object files
$(BIN)%.o: %.c 
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	-rm -rf $(BIN)

# Optional: Provide a phony target to avoid conflicts with files named 'clean'
.PHONY: all clean