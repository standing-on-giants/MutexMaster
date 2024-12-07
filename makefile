# Compiler to use
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -I.

# Linker flags
LDFLAGS = -lcunit

# Source files
SRCS_SERVER = ser.c mem.c adm.c routes.c
SRCS_CLIENT = cli.c
SRCS_TEST = test.c routes.c

# Header files
HEADERS = auth.h 
HEADERS_TEST =auth.h test.h

# Output executables
TARGET_SERVER = server
TARGET_CLIENT = client
TARGET_TEST = test


# Default rule to build everything except test
all: $(TARGET_SERVER) $(TARGET_CLIENT)

# Rule to build the server executable
$(TARGET_SERVER): $(SRCS_SERVER) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SRCS_SERVER)

# Rule to build the client executable
$(TARGET_CLIENT): $(SRCS_CLIENT) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SRCS_CLIENT)

# Rule to build test
test: $(TARGET_TEST)
$(TARGET_TEST): $(SRCS_TEST) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SRCS_TEST) $(LDFLAGS)
	./$(TARGET_TEST)

# Rule to clean the build artifacts/ executables
clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT)

