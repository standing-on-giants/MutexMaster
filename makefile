# Compiler to use
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -I.

# Source files
SRCS_SERVER = ser.c mem.c adm.c
SRCS_CLIENT = cli.c

# Header files
HEADERS = auth.h

# Output executables
TARGET_SERVER = server
TARGET_CLIENT = client

# Default rule to build everything
all: $(TARGET_SERVER) $(TARGET_CLIENT)

# Rule to build the server executable
$(TARGET_SERVER): $(SRCS_SERVER) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SRCS_SERVER)

# Rule to build the client executable
$(TARGET_CLIENT): $(SRCS_CLIENT) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SRCS_CLIENT)

# Rule to clean the build artifacts/ executables
clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT)

