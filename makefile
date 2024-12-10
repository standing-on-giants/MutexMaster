CC = gcc

CFLAGS = -Wall -w -Wextra -I.

# Linker flags
LDFLAGS = -lcunit

# Source files
SRCS_SERVER = ser_files/ser.c ser_files/mem.c ser_files/adm.c ser_files/routes_server.c auth.c
SRCS_CLIENT = cli_files/cli.c cli_files/routes_client.c auth.c
SRCS_TEST_SERVER = test_files/test_ser.c ser_files/mem.c ser_files/adm.c ser_files/routes_server.c auth.c
SRCS_TEST_CLIENT = test_files/test_cli.c  cli_files/routes_client.c auth.c
SRCS_TEST = test_files/test.c auth.c ser_files/mem.c ser_files/adm.c ser_files/routes_server.c cli_files/routes_client.c

# Header files
HEADERS_CLIENT = auth.h cli_files/cli.h
HEADERS_SERVER = auth.h ser_files/ser.h
HEADERS_TEST =auth.h test_files/test.h ser_files/ser.h cli_files/cli.h

# Output executables
TARGET_SERVER = server
TARGET_CLIENT = client
TARGET_TEST = test
TARGET_TEST_SERVER = test_server
TARGET_TEST_CLIENT = test_client


# rule to build everything except test
all: $(TARGET_SERVER) $(TARGET_CLIENT)

# server executable
$(TARGET_SERVER): $(SRCS_SERVER) $(HEADERS_SERVER)
	$(CC) $(CFLAGS) -o $@ $(SRCS_SERVER)

# client executable
$(TARGET_CLIENT): $(SRCS_CLIENT) $(HEADERS_CLIENT)
	$(CC) $(CFLAGS) -o $@ $(SRCS_CLIENT)

# Rule to build test
test:  $(TARGET_TEST) $(TARGET_TEST_SERVER) #$(TARGET_TEST_CLIENT) 

$(TARGET_TEST): $(SRCS_TEST) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SRCS_TEST) $(LDFLAGS)
	./$(TARGET_TEST)

$(TARGET_TEST_SERVER): $(SRCS_TEST_SERVER) $(HEADERS_TEST)
	$(CC) $(CFLAGS) -o $@ $(SRCS_TEST_SERVER) $(LDFLAGS)
	./$(TARGET_TEST_SERVER)

$(TARGET_TEST_CLIENT): $(SRCS_TEST_CLIENT) $(HEADERS_TEST)
	$(CC) $(CFLAGS) -o $@ $(SRCS_TEST_CLIENT) $(LDFLAGS)
	./$(TARGET_TEST_CLIENT)

clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT) 

cleant:
	rm -f $(TARGET_TEST) $(TARGET_TEST_SERVER)


