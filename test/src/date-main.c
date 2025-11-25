#include "../../src/parse-http-date.h"
#include <string.h>
#include <stdio.h>

/* For older compilers that don't define these */
#ifndef __bool_true_false_are_defined
#define bool int
#define true 1
#define false 0
#endif

#define VALID_MINHTTPD  0x0001
#define VALID_APACHE    0x0002
#define VALID_NGINX     0x0004
#define VALID_LITESPD   0x0008


struct testcases_t {
    time_t expected_result;
    const char *string;
    unsigned validity;
    bool is_until_crlf:1; /* parsed until end-of-line */
} testcases[] = {

    { 784111777, "Sun, 06 Nov 1994 08:49:37 GMT",   0xFF, 0},
    { 784111777, "Sunday, 06-Nov-94 08:49:37 GMT",  0xFF, 0},
    { 784111777, "Sun Nov  6 08:49:37 1994",        0xF7, 0},
    { 784111777, "Sun Nov 06 08:49:37 1994",        0xF7, 0},

    /* Some tests from Apache for incorrect formats. They should
     * fail our parser*/
#if 0
    { 793946984, "Mon, 27 Feb 1995 20:49:44 -0800",   2|4, 0}, //"Tue, 28 Feb 1995 04:49:44 GMT"
    {1120232065, "Fri,  1 Jul 2005 11:34:25 -0400",   2, 0}, //"Fri, 01 Jul 2005 15:34:25 GMT"
    { 793946984, "Monday, 27-Feb-95 20:49:44 -0800",  2, 0}, //"Tue, 28 Feb 1995 04:49:44 GMT"
    { 857472232, "Tue, 4 Mar 1997 12:43:52 +0200",    2, 0}, //"Tue, 04 Mar 1997 10:43:52 GMT"
    { 793946984, "Mon, 27 Feb 95 20:49:44 -0800",     2, 0}, //"Tue, 28 Feb 1995 04:49:44 GMT"
    { 857472232, "Tue,  4 Mar 97 12:43:52 +0200",     2, 0}, //"Tue, 04 Mar 1997 10:43:52 GMT"
    { 857472232, "Tue, 4 Mar 97 12:43:52 +0200",      2, 0}, //"Tue, 04 Mar 1997 10:43:52 GMT"
    { 793918140, "Mon, 27 Feb 95 20:49 GMT",          2, 0}, //"Mon, 27 Feb 1995 20:49:00 GMT"
    { 857479380, "Tue, 4 Mar 97 12:43 GMT",           2, 0}, //"Tue, 04 Mar 1997 12:43:00 GMT"
#endif
    
    /* 0 - Epoch */
    { 0, "Thu, 01 Jan 1970 00:00:00 GMT",           0xF5, 0},
    { 0, "Thursday, 01-Jan-70 00:00:00 GMT",        0xF5, 0},
    { 0, "Thu Jan  1 00:00:00 1970",                0xF5, 0},
  
    /* 1 - Epoch plus 1 */
    { 1, "Fri, 01 Jan 1970 00:00:01 GMT",           0xFF, 0},
    { 1, "Friday, 01-Jan-70 00:00:01 GMT",          0xFF, 0},
    { 1, "Fri Jan  1 00:00:01 1970",                0xF7, 0},
  
#if 0
    /* 946684799 - 1999-12-31 23:59:59 UTC */
    { 946684799, "Fri, 31 Dec 1999 23:59:59 GMT",   0xFF, 0},
    { 946684799, "Friday, 31-Dec-99 23:59:59 GMT",  0xFF, 0},
    { 946684799, "Fri Dec 31 23:59:59 1999",        0xFF, 0},
    
    /* 951827696 - 2000-02-29 12:34:56 UTC */
    { 951827696, "Tue, 29 Feb 2000 12:34:56 GMT",   0xFF, 0},
    { 951827696, "Tuesday, 29-Feb-00 12:34:56 GMT", 0xFF, 0},
    { 951827696, "Tue Feb 29 12:34:56 2000",        0xFF, 0},
    
    /* 1330559999 - 2012-02-29 23:59:59 UTC */
    { 1330559999, "Wed, 29 Feb 2012 23:59:59 GMT",  0xFF, 0},
    { 1330559999, "Wednesday, 29-Feb-12 23:59:59 GMT",0xFF,0},
    { 1330559999, "Wed Feb 29 23:59:59 2012",       0xFF, 0},
    
    /* 1262401445 - 2010-01-02 03:04:05 UTC */
    { 1262401445, "Sat, 02 Jan 2010 03:04:05 GMT",  0xFF, 0},
    { 1262401445, "Saturday, 02-Jan-10 03:04:05 GMT",0xFF,0},
    { 1262401445, "Sat Jan  2 03:04:05 2010",       0xFF, 0},
    
    /* 2147483647 - 2038-01-19 03:14:07 UTC (Y2038 before) */
    { 2147483647, "Tue, 19 Jan 2038 03:14:07 GMT",  0xFF, 0},
    { 2147483647, "Tuesday, 19-Jan-38 03:14:07 GMT",0xFF, 0},
    { 2147483647, "Tue Jan 19 03:14:07 2038",       0xFF, 0},
    
    /* 2147483647 - 2038-01-19 03:14:07 UTC (Y2038 after) */
    { 2147483648, "Tue, 19 Jan 2038 03:14:08 GMT",  0xFF, 0},
    { 2147483648, "Tuesday, 19-Jan-38 03:14:08 GMT",0xFF, 0},
    { 2147483648, "Tue Jan 19 03:14:08 2038",       0xFF, 0},
#endif
    {0,0}
};

struct testcases_t crlf_testcases[] = {
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


    {784111777, "Sun, 31 Nov 1994 08:49:37 GMT"},
    {0,0}
};

time_t apr_date_parse_rfc(const char *date);
time_t ngx_http_parse_time(const char *value, size_t len);
time_t litespeed_parseHttpTime(const char *s, int len);

/**
 * Runs a single testcase with parser differentials
 */
int test_differential(size_t i) {
    time_t expected = testcases[i].expected_result;
    const char *string = testcases[i].string;
    unsigned validity = testcases[i].validity;
    size_t string_length = strlen(string);
    struct HttpDate date;
    unsigned state = 0;
    time_t result;
    char status[16] = ".....";

    /*
     * MinHttpd
     */
    state = parse_http_date(state, string, &string_length, &date, 0);
    result = date.timestamp;
    if ((validity & VALID_MINHTTPD) && result == expected) {
        status[0] = 'M';
    } else if (!(validity & VALID_MINHTTPD) && result == -1) {
        status[0] = 'm';
    } else {
        status[0] = '-';
        if (!(validity & VALID_MINHTTPD))
            expected = -1;
        fprintf(stderr, "[-] %3d:M: %s, expected %lld, got %lld\n", (int)i, string, (long long)expected, (long long)result);
        return 1;
    }

    /*
     * Apache
     */
    result = apr_date_parse_rfc(string);
    if (result == 0)
        result = -1;
    if ((validity & VALID_APACHE) && result == expected) {
        status[1] = 'A';
    } else if (!(validity & VALID_APACHE) && result == -1) {
        status[1] = 'a';
    } else {
        status[1] = '-';
        /*if (!(validity & VALID_APACHE))
            expected = -1;
        fprintf(stderr, "[-] %3d:A: %s, expected %llu, got %llu\n", (int)i, string, (long long)expected, (long long)result);
        return 1; */
    }

    /*
     * Nginx
     */
    result = ngx_http_parse_time(string, string_length);
    if ((validity & VALID_NGINX) && result == expected) {
        status[2] = 'N';
    } else if (!(validity & VALID_NGINX) && result == -1) {
        status[2] = 'n';
    } else {
        status[2] = '-';
        /*if (!(validity & VALID_NGINX))
            expected = -1;
        fprintf(stderr, "[-] %3d:N: %s, expected %llu, got %llu\n", (int)i, string, (long long)expected, (long long)result);
        return 1; */
    }

    /*
     * LiteSpeed (OpenLiteSpeed)
     */
    result = litespeed_parseHttpTime(string, (int)string_length);
    if (result == 0)
        result = -1;
    if ((validity & VALID_LITESPD) && result == expected) {
        status[3] = 'L';
    } else if (!(validity & VALID_LITESPD) && result == -1) {
        status[3] = 'l';
    } else {
        status[3] = '-';
        /*if (!(validity & VALID_LITESPD))
            expected = -1;
        fprintf(stderr, "[-] %3d:L: %s, expected %llu, got %llu\n", (int)i, string, (long long)expected, (long long)result);
        return 1;*/
    }


    printf("[%s] %s\n", status, string);
    
    return 0;
}

#if 0
/* Test we've processed all the data that we should've */
if (date.is_until_crlf) {
    expected_length = string_length - 1;
}
if (expected_length && expected_length != string_length) {
    fprintf(stderr, "[-] %s: expected length = %d, found length = %d\n",
        string, (int)expected_length, (int)string_length);
}

#endif

int test_bad(const char *string, size_t offset) {
    time_t expected = -1;
    unsigned validity = 0;
    size_t string_length = strlen(string);
    struct HttpDate date;
    unsigned state = 0;
    time_t result;
    
    /*
     * MinHttpd
     */
    state = parse_http_date(state, string, &string_length, &date, 0);
    result = date.timestamp;
    if (result != -1) {
        /* this is supposed to fail */
        fprintf(stderr, "[-] %s, expected %lld, got %lld\n", string, (long long)expected, (long long)result);
        return 1;
    }
    fprintf(stderr, "[+] %s\n", string);
    fprintf(stderr, "    %.*s^\n", (int)string_length, "                           ");
    fprintf(stderr, "    %.*s|\n", (int)offset, "                           ");

    return 0;
}

int main(int argc, char *argv[]) {
    size_t i;
    const char *fmt1 = "Sun, 06 Nov 1994 08:49:37 GMT";
    
    init_http_date_parser();
    
    for (i=0; fmt1[i]; i++) {
        char buf[1024];
        memcpy(buf, fmt1, strlen(fmt1)+1);
        
        buf[i] = 'X';
        test_bad(buf, i);
    }

    for (i=0; testcases[i].string; i++) {
        test_differential(i);
    }
    fprintf(stderr, "[+] done\n");
}

