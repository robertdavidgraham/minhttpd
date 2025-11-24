#include "../../src/parse-http-date.h"
#include <string.h>
#include <stdio.h>

struct {
    time_t expected_result;
    const char *string;
} testcases[] = {
    {784111777, "Sun, 32 Nov 1994 08:49:37 GMT"},
    {784111777, "Sun, 06 Nov 1994 08:49:37 GMT"},
    {784111777, "Sun, 06 Nov 1994 08:49:37 GMT"},
    {784111777, "Sun, 06 Nov 1994 08:49:37 GMT"},
    {784111777, "Sun, 06 Nov 1994 08:49:37 GMT"},
    {784111777, "Sun, 32 Nov 1994 08:49:37 GMT"},
    {0,0}
};

int run_test_case(size_t i) {
    time_t expected = testcases[i].expected_result;
    const char *string = testcases[i].string;
    size_t string_length = strlen(string);
    struct HttpDate date;
    unsigned state = 0;
    time_t result;
    
    state = parse_http_date(state, string, &string_length, &date, 0);
    result = date.timestamp;
    
    if (state == DATE_INVALID) {
        if (expected == -1) {
            fprintf(stderr, "[+] %d: %lld == %s\n", (int)i, (long long)result, string);
            return 0; /* success, we expected parse failure */
        } else {
            fprintf(stderr, "[-] %d: error: %s\n", (int)i, string);
            return 1; /* failure */
        }
    } else if (state == DATE_VALID) {
        if (expected == -1) {
            fprintf(stderr, "[-] %d: error: %s, expected failure, got %llu\n", (int)i, string, (long long)result);
            return 1; /* failure, should have failed to parse */
        } else if (expected != result) {
            fprintf(stderr, "[-] %d: %lld != %s: expected %llu\n", (int)i, (long long)result, string, (long long)expected);
            return 1; /* failure */
        } else {
            fprintf(stderr, "[+] %d: %lld == %s\n", (int)i, (long long)result, string);
            return 0;
        }
        return 0;
    } else {
        fprintf(stderr, "[-] %d: %lld != %s: BAD PARSING\n", (int)i, (long long)result, string);
        return 1; /* not possible */
    }
}


int main(int argc, char *argv[]) {
    size_t i;

    init_http_date_parser();
    

    for (i=0; testcases[i].string; i++) {
        run_test_case(i);
    }
    fprintf(stderr, "[+] done\n");
}

