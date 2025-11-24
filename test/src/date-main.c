#include "../../src/parse-http-date.h"
#include <string.h>
#include <stdio.h>

struct {
    time_t expected_result;
    const char *string;
    int is_until_crlf;
} testcases[] = {
    /* Some tests from Apache for incorrect formats. They should
      * fail our parser*/
    { 793946984, "Mon, 27 Feb 1995 20:49:44 -0800",   0}, //"Tue, 28 Feb 1995 04:49:44 GMT"
    {1120232065, "Fri,  1 Jul 2005 11:34:25 -0400",   0}, //"Fri, 01 Jul 2005 15:34:25 GMT"
    { 793946984, "Monday, 27-Feb-95 20:49:44 -0800",  0}, //"Tue, 28 Feb 1995 04:49:44 GMT"
    { 857472232, "Tue, 4 Mar 1997 12:43:52 +0200",    0}, //"Tue, 04 Mar 1997 10:43:52 GMT"
    { 793946984, "Mon, 27 Feb 95 20:49:44 -0800",     0}, //"Tue, 28 Feb 1995 04:49:44 GMT"
    { 857472232, "Tue,  4 Mar 97 12:43:52 +0200",     0}, //"Tue, 04 Mar 1997 10:43:52 GMT"
    { 857472232, "Tue, 4 Mar 97 12:43:52 +0200",      0}, //"Tue, 04 Mar 1997 10:43:52 GMT"
    { 793918140, "Mon, 27 Feb 95 20:49 GMT",          0}, //"Mon, 27 Feb 1995 20:49:00 GMT"
    { 857479380, "Tue, 4 Mar 97 12:43 GMT",           0}, //"Tue, 04 Mar 1997 12:43:00 GMT"

    { 784111777, "Sun, 06 Nov 1994 08:49:37 GMT" },
    { 784111777, "Sunday, 06-Nov-94 08:49:37 GMT" },
    { 784111777, "Sun Nov  6 08:49:37 1994" },

    /* 0 — Epoch */
    { 0, "Thu, 01 Jan 1970 00:00:00 GMT" },
    { 0, "Thursday, 01-Jan-70 00:00:00 GMT" },
    { 0, "Thu Jan  1 00:00:00 1970" },

    /* 946684799 — 1999-12-31 23:59:59 UTC */
    { 946684799, "Fri, 31 Dec 1999 23:59:59 GMT" },
    { 946684799, "Friday, 31-Dec-99 23:59:59 GMT" },
    { 946684799, "Fri Dec 31 23:59:59 1999" },

    /* 951827696 — 2000-02-29 12:34:56 UTC */
    { 951827696, "Tue, 29 Feb 2000 12:34:56 GMT" },
    { 951827696, "Tuesday, 29-Feb-00 12:34:56 GMT" },
    { 951827696, "Tue Feb 29 12:34:56 2000" },

    /* 1330559999 — 2012-02-29 23:59:59 UTC */
    { 1330559999, "Wed, 29 Feb 2012 23:59:59 GMT" },
    { 1330559999, "Wednesday, 29-Feb-12 23:59:59 GMT" },
    { 1330559999, "Wed Feb 29 23:59:59 2012" },

    /* 1262401445 — 2010-01-02 03:04:05 UTC */
    { 1262401445, "Sat, 02 Jan 2010 03:04:05 GMT" },
    { 1262401445, "Saturday, 02-Jan-10 03:04:05 GMT" },
    { 1262401445, "Sat Jan  2 03:04:05 2010" },

    /* 2147483647 — 2038-01-19 03:14:07 UTC (Y2038 boundary) */
    { 2147483647, "Tue, 19 Jan 2038 03:14:07 GMT" },
    { 2147483647, "Tuesday, 19-Jan-38 03:14:07 GMT" },
    { 2147483647, "Tue Jan 19 03:14:07 2038" },

    {784111777, "Sun, 06 Nov 1994 08:49:37 GMT\r\n*", 1},
    {784111777, "Sunday, 06-Nov-94 08:49:37 GMT\r\n*", 1},
    {784111777, "Sun Nov  6 08:49:37 1994\r\n*", 1},
    {784111777, "Sun Nov 06 08:49:37 1994\r\n*", 1},

    {784111777, "Sun, 06 Nov 1994 08:49:37 GMT \r\n*", 1},
    {784111777, "Sunday, 06-Nov-94 08:49:37 GMT  \r\n*", 1},
    {784111777, "Sun Nov  6 08:49:37 1994 \t \r\n*", 1},
    {784111777, "Sun Nov 06 08:49:37 1994 \t\t\r\n*", 1},

    {784111777, "Sun, 06 Nov 1994 08:49:37 GMT", 0},
    {784111777, "Sunday, 06-Nov-94 08:49:37 GMT", 0},
    {784111777, "Sun Nov  6 08:49:37 1994", 0},
    {784111777, "Sun Nov 06 08:49:37 1994", 0},


    {-1, "Sun, 31 Nov 1994 08:49:37 GMT"},
    {0,0}
};

time_t apr_date_parse_rfc(const char *date);

int run_test_case(size_t i) {
    time_t expected = testcases[i].expected_result;
    const char *string = testcases[i].string;
    size_t string_length = strlen(string);
    size_t expected_length = 0;
    struct HttpDate date;
    unsigned state = 0;
    time_t result;


    result = apr_date_parse_rfc(string);
    if (result == expected) {
        fprintf(stderr, "[+] %d: %lld == %.*s\n", (int)i, (long long)result, (int)string_length, string);
        return 0;
    } else {
        fprintf(stderr, "[-] %d: %lld != %.*s\n", (int)i, (long long)result, (int)string_length, string);
        return 1;
    }

    /* Test whether we are parsing until the end-of-line */
    date.is_until_crlf = testcases[i].is_until_crlf;

    /* Test we've processed all the data that we should've */
    if (date.is_until_crlf) {
        expected_length = string_length - 1;
    }


    state = parse_http_date(state, string, &string_length, &date, date.is_until_crlf);
    result = date.timestamp;

    if (expected_length && expected_length != string_length) {
        fprintf(stderr, "[-] %s: expected length = %d, found length = %d\n",
            string, (int)expected_length, (int)string_length);
    }
    
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
            fprintf(stderr, "[+] %d: %lld == %.*s\n", (int)i, (long long)result, (int)string_length, string);
            return 0;
        }
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

