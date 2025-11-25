#define _CRT_SECURE_NO_WARNINGS
#include "../../src/parse-http-date.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>

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
time_t lighthttp_parse_date(const char *buf, uint32_t len);

int test1(const char *string, time_t expected, unsigned validity, char *status) {
    size_t string_length = strlen(string);
    struct HttpDate date;
    unsigned state = 0;
    time_t result;

    /*
     * MinHttpd
     */
    state = parse_http_date(state, string, &string_length, &date, 0);
    result = date.timestamp;
    if (result == expected && expected != -1) {
        status[0] = 'M';
    } else if (result == -1) {
        status[0] = '.';
    } else {
        status[0] = '-';
        if (!(validity & VALID_MINHTTPD))
            expected = -1;
        fprintf(stderr, "[-] M: %s, expected %lld, got %lld\n", string, (long long)expected, (long long)result);
        return 1;
    }

    /*
     * Apache
     */
    result = apr_date_parse_rfc(string);
    if (result == 0)
        result = -1;
    if (result == expected && expected != -1) {
        status[1] = 'A';
    } else if (result == -1) {
        status[1] = '.';
    } else {
        status[1] = '-';
    }

    /*
     * Nginx
     */
    result = ngx_http_parse_time(string, string_length);
    if (result == expected && expected != -1) {
        status[2] = 'N';
    } else if (result == -1) {
        status[2] = '.';
    } else {
        status[2] = '-';
    }

    /*
     * LiteSpeed (OpenLiteSpeed)
     */
    result = litespeed_parseHttpTime(string, (int)string_length);
    if (result == 0)
        result = -1;
    if (result == expected && expected != -1) {
        status[3] = 'L';
    } else if (result == -1) {
        status[3] = '.';
    } else {
        status[3] = '-';
    }

    /*
     * LightHTTPD (Fly Light)
     */
    result = lighthttp_parse_date(string, (unsigned)string_length);
    if (result == 0)
        result = -1;
    if (result == expected && expected != -1) {
        status[4] = 'F';
    } else if (result == -1) {
        status[4] = '.';
    } else {
        status[4] = '-';
    }

    status[5] = '\0';
    //printf("[%s] %s\n", status, string);
    
    return 0;
}



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

int test_bad(const char *string, size_t offset, time_t expected) {
    size_t string_length = strlen(string);
    size_t original_length = string_length;
    struct HttpDate date;
    unsigned state = 0;
    time_t result;
    char status[16];
    
    /*
     * MinHttpd
     */
    state = parse_http_date(state, string, &string_length, &date, 0);
    result = date.timestamp;
    if (result != -1 && expected != expected) {
        /* this is supposed to fail */
        fprintf(stderr, "[-] %s, expected %lld, got %lld\n", string, (long long)expected, (long long)result);
        return 1;
    }
    /*if (result == -1 && string_length < original_length)
        string[string_length] = '*';*/

    test1(string, expected, 0, status);
    fprintf(stdout, "[%s] 0x%09llx %s\n", status, expected, string);

    return 0;
}

/**
 * This test bad variations of good strings. It simply takes the 'x'
 * character and replaces it in the string, replacing one character
 * at a time.
 *
 * This verifies that the string fails, but also, it verifies the 
 * location of the failure, to make sure it's been detected correct.
 * One of the features of a state-machine parser is that we have a simpler
 * way of knowing where the failure was in the string.
 */
static int
test_bads(void) {
    static const char *bads[] = {
        "Sun, 06 Nov 1994 08:49:37 GMT\0\0\0\0",
        "Sunday, 06-Nov-94 08:49:37 GMT",
        "Sun Nov  6 08:49:37 1994",
        0
    };
    size_t i;


    /* for all patterns */
    for (i=0; bads[i]; i++) {
        const char *bad = bads[i];
        size_t j;

        /* First do a good one */
        test_bad(bad, strlen(bad), 784111777);

        /* for all mutations of the pattern */
        for (j=0; bad[j]; j++) {
            char buf[1024];
            int err;

            memcpy(buf, bad, strlen(bad)+1);
            buf[j] = 'X';
            
            err = test_bad(buf, j, 784111777);
            if (err)
                return err;
        }
    }
    return 0; /* success */
}

static int
test_goods(void) {
    char buf[1024];
    static const time_t goods[] = {
        0,
        1,
        0x02ebc98a1, // Sun, 06 Nov 1994 08:49:37 GMT
        0x0386d437e,
        0x0386d437f,
        0x0386d4380,
        0x0386d4381,
        0x04b3eb7a5, //  Sat, 02 Jan 2010 03:04:05 GMT
        0x07ffffffe, // - 2038-01-19 03:14:07 UTC (Y2038 after) */
        0x07fffffff, // - 2038-01-19 03:14:07 UTC (Y2038 after) */
        0x080000000, //"Tue, 19 Jan 2038 03:14:08 GMT",  0xFF, 0},
        0x080000001, //"Tue, 19 Jan 2038 03:14:08 GMT",  0xFF, 0},
        -1
    };
    size_t i;


    /* RFC1123/IMF-time */
    for (i=0; goods[i] != -1; i++) {
        time_t good = goods[i];
        struct tm *tm;
        size_t length;

        tm = gmtime(&good);
        length = strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", tm);
        if (length == 0) {
            fprintf(stderr, "[-] strftime() error\n");
            continue;
        }

        test_bad(buf, length, good);
    }

    /* RFC850 */
    for (i=0; goods[i] != -1; i++) {
        time_t good = goods[i];
        struct tm *tm;
        size_t length;

        tm = gmtime(&good);
        length = strftime(buf, sizeof(buf), "%A, %d-%b-%y %H:%M:%S GMT", tm);
        if (length == 0) {
            fprintf(stderr, "[-] strftime() error\n");
            continue;
        }

        test_bad(buf, length, good);
    }

    /* asctime() */
    for (i=0; goods[i] != -1; i++) {
        time_t good = goods[i];
        struct tm *tm;
        size_t length;
        const char *buf2;
        

        tm = gmtime(&good);
        buf2 = asctime(tm);
        strcpy(buf, buf2);
        
        while (buf[0] && isspace(buf[strlen(buf)-1]&0xFF))
            buf[strlen(buf)-1] = '\0'; /*strip trailing whitespace*/
        

        test_bad(buf, strlen(buf), good);
    }


    return 0; /* success */
}

/* For LightHTTPD */
time_t log_epoch_secs;

int main(int argc, char *argv[]) {
    int i;
    int errs = 0;
    int test_count = 0;
    
    /* LightHTTPD init */
    log_epoch_secs = time(0);

    /* MinHTTPD init */
    init_http_date_parser();

    /*
     * Loop through commands
     */
    for (i=1; i<argc; i++) {
        const char *arg = argv[i];

        if (strcmp("--bad", arg) == 0) {
            errs += test_bads();
            test_count++;
        } else if (strcmp("--good", arg) == 0) {
            errs += test_goods();
            test_count++;
        }
    }

    /* Make sure we executed at least one test, or else
     * print help. */
    if (test_count == 0) {
        fprintf(stderr, "[-] no tests ran\n");
        fprintf(stderr, "usage:\n date-test <test1> <test2>...\n");
        fprintf(stderr, "where some tests are:\n");
        fprintf(stderr, " --bad\n");
        fprintf(stderr, " --good\n");
    }

    if (errs)
        fprintf(stderr, "[-] %d errors\n", errs);
    else
        fprintf(stderr, "[+] success!\n");
    return errs;
}

