#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <fcntl.h>
#include "auth.h"
#include <time.h>
#include <asm-generic/socket.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

// Test case for format_time
void test_format_time(void) {
    time_t rawtime = 1672531200; // Equivalent to "2023-01-01 00:00:00 UTC"
    char buffer[20];
    format_time(rawtime, buffer, sizeof(buffer));

    // Expected output for the given time
    const char *expected_time = "2023-01-01 00:00:00";

    // Assert the result
    CU_ASSERT_STRING_EQUAL(buffer, expected_time);
}

int main() {
    // Initialize the CUnit test registry
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    // Create a test suite
    CU_pSuite pSuite = CU_add_suite("Format Time Suite", NULL, NULL);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add the test case to the suite
    if (NULL == CU_add_test(pSuite, "test of format_time()", test_format_time)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Run all tests using the basic interface
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
