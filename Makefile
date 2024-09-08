# Define the compiler and flags
CC = g++
CFLAGS = -fPIC -O3 -Ofast -I./include/
LDFLAGS = -shared
RM = rm -f

# Define the target library name
TARGET_LIB = lib__ypp.so

# Define the source files
SRCS = src/main.cpp

# Define the object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET_LIB)

# Rule to create the shared library
$(TARGET_LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Rule to compile source files to object files
$(OBJS): ${SRCS}
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	$(RM) $(OBJS) $(TARGET_LIB)

install: $(TARGET_LIB) include/PreY
	sudo cp $(TARGET_LIB) /usr/local/lib/
	sudo cp include/PreY /usr/local/include/
	sudo cp include/YDebug /usr/local/include/

.PHONY: all clean
