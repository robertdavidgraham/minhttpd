#include "../src/parse-http-date.h"
#include <string.h>
#include <stdio.h>

struct {
    time_t expected_result;
    const char *string;
} testcases[] = {
    {0, "Sun, 06 Nov 1994 08:49:37 GMT"},
    {0,0}
};

int main(int argc, char *argv[]) {
    size_t i;

    init_http_date_parser();
    

    for (i=0; testcases[i].string; i++) {
        const char *string = testcases[i].string;
        size_t string_length = strlen(string);
        struct HttpDate date;
        unsigned state = 0;

        state = parse_http_date(state, string, &string_length, &date, 0);
        if (state == DATE_INVALID) {
            fprintf(stderr, "[-] error\n");
            break;
        } else if (state == DATE_VALID) {
            fprintf(stderr, "[+] success\n");
        }
    }
    fprintf(stderr, "[+] done\n");
}

