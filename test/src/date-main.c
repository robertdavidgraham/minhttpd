/*
    Test program for parsing "Date:" field.
 
 This is a simple test program:
    --good - tests several good strings, getting the right result
    --bad - verifies that rejects badly formatted strings
    --date <string> - prints output from a single input string on command-line
    <filename> - reads from a file, for fuzzing
 */
#define _CRT_SECURE_NO_WARNINGS
#include "../../src/parse-http-date.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>

/* For older compilers that don't define these */
#ifndef __bool_true_false_are_defined
#define bool int
#define true 1
#define false 0
#endif



struct testcases_t {
    time_t expected_result;
    const char *string;
} testcases[] = {

    { 784111777, "Sun, 06 Nov 1994 08:49:37 GMT"},
    { 784111777, "Sunday, 06-Nov-94 08:49:37 GMT"},
    { 784111777, "Sun Nov  6 08:49:37 1994"},
    { 784111777, "Sun Nov 06 08:49:37 1994"},


    
  
#if 0
    /* 946684799 - 1999-12-31 23:59:59 UTC */
    { 946684799, "Fri, 31 Dec 1999 23:59:59 GMT"},
    { 946684799, "Friday, 31-Dec-99 23:59:59 GMT"},
    { 946684799, "Fri Dec 31 23:59:59 1999"},
    
    /* 951827696 - 2000-02-29 12:34:56 UTC */
    { 951827696, "Tue, 29 Feb 2000 12:34:56 GMT"},
    { 951827696, "Tuesday, 29-Feb-00 12:34:56 GMT"},
    { 951827696, "Tue Feb 29 12:34:56 2000"},
    
    /* 1330559999 - 2012-02-29 23:59:59 UTC */
    { 1330559999, "Wed, 29 Feb 2012 23:59:59 GMT"},
    { 1330559999, "Wednesday, 29-Feb-12 23:59:59 GMT"},
    { 1330559999, "Wed Feb 29 23:59:59 2012"},
    
    /* 1262401445 - 2010-01-02 03:04:05 UTC */
    { 1262401445, "Sat, 02 Jan 2010 03:04:05 GMT"},
    { 1262401445, "Saturday, 02-Jan-10 03:04:05 GMT"},
    { 1262401445, "Sat Jan  2 03:04:05 2010"},
    
    /* 2147483647 - 2038-01-19 03:14:07 UTC (Y2038 before) */
    { 2147483647, "Tue, 19 Jan 2038 03:14:07 GMT"},
    { 2147483647, "Tuesday, 19-Jan-38 03:14:07 GMT"},
    { 2147483647, "Tue Jan 19 03:14:07 2038"},
    
    /* 2147483647 - 2038-01-19 03:14:07 UTC (Y2038 after) */
    { 2147483648, "Tue, 19 Jan 2038 03:14:08 GMT"},
    { 2147483648, "Tuesday, 19-Jan-38 03:14:08 GMT"},
    { 2147483648, "Tue Jan 19 03:14:08 2038"},
#endif
    {0,0}
};

struct testcases_t crlf_testcases[] = {
    {784111777, "Sun, 06 Nov 1994 08:49:37 GMT\r\n*"},
    {784111777, "Sunday, 06-Nov-94 08:49:37 GMT\r\n*"},
    {784111777, "Sun Nov  6 08:49:37 1994\r\n*"},
    {784111777, "Sun Nov 06 08:49:37 1994\r\n*"},

    {784111777, "Sun, 06 Nov 1994 08:49:37 GMT \r\n*"},
    {784111777, "Sunday, 06-Nov-94 08:49:37 GMT  \r\n*"},
    {784111777, "Sun Nov  6 08:49:37 1994 \t \r\n*"},
    {784111777, "Sun Nov 06 08:49:37 1994 \t\t\r\n*"},

    {0,0}
};

/*
 * These are defined in files like date-apache.c, date-nginx.c,
 * and so forth.
 */
time_t apr_date_parse_rfc(const char *date);
time_t ngx_http_parse_time(const char *value, size_t len);
time_t litespeed_parseHttpTime(const char *s, int len);
time_t lighthttp_parse_date(const char *buf, uint32_t len);
time_t simple_parse_date(const char *s);


/**
 * This tests the string input across many code bases to look for "parser differentials",
 * where different web servers may treat this field differently.
 */
int test1(const char *string, size_t length, time_t expected, char *status) {
    struct HttpDate scratch;
    time_t result;

    /*
     * MinHttpd
     */
    result = parse_http_date(string, length);
    if (result == expected && expected != -1) {
        status[0] = 'M';
    } else if (result == -1) {
        status[0] = '.';
    } else {
        status[0] = '-';
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
    result = ngx_http_parse_time(string, length);
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
    result = litespeed_parseHttpTime(string, (int)length);
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
    result = lighthttp_parse_date(string, (unsigned)length);
    if (result == 0)
        result = -1;
    if (result == expected && expected != -1) {
        status[4] = 'F';
    } else if (result == -1) {
        status[4] = '.';
    } else {
        status[4] = '-';
    }

    /*
     * Simple parser
     */
    result = simple_parse_date(string);
    if (result == expected && expected != -1) {
        status[5] = 'S';
    } else if (result == -1) {
        status[5] = '.';
    } else {
        status[5] = '-';
    }

    /* nul-termiante string */
    status[6] = '\0';
    
    return 0;
}



int test_bad(const char *string, time_t expected) {
    size_t string_length = strlen(string);
    size_t original_length = string_length;
    char status[16];


    test1(string, original_length, expected, status);
    fprintf(stdout, "[%s] 0x%09llx %s\n", status, (long long)expected, string);

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
    /* These are bad strings that others might accept that we
     * reject. Specifically, some are from the Apache test
     * cases, where they accept timezones unlike everyone else */
    static const struct testcases_t bads[] = {
        {857479380,                 "Tue, 4 Mar 97 12:43 GMT"},
        {1120232065,                "Fri,  1 Jul 2005 11:34:25 -0400"},
        {0,0}
    };

    /* These test when integers go out of bounds, like 31 days for
     * November, a month that has only 30 days. Of particular interest
     * is testing the leap-second that can be 60. */
    static const struct testcases_t badnums[] = {
        {784111777 + 25*24*60*60,   "Thu, 31 Nov 1994 08:49:37 GMT"},
        {784111777 + 26*24*60*60,   "Fri, 32 Nov 1994 08:49:37 GMT"},
        {784111777 + 16*60*60,      "Sun, 06 Nov 1994 24:49:37 GMT"},
        {784111777 + 60,            "Sun, 06 Nov 1994 08:60:37 GMT"},
        {784111777 + 23,            "Sun, 06 Nov 1994 08:49:60 GMT"},
        {784111777 + 24,            "Sun, 06 Nov 1994 08:49:61 GMT"},
        {784111777 + 25*24*60*60,   "Thursday, 31-Nov-94 08:49:37 GMT"},
        {784111777 + 26*24*60*60,   "Tuesday, 32-Nov-94 08:49:37 GMT"},
        {784111777 + 16*60*60,      "Sunday, 06-Nov-94 24:49:37 GMT"},
        {784111777 + 60,            "Sunday, 06-Nov-94 08:60:37 GMT"},
        {784111777 + 23,            "Sunday, 06-Nov-94 08:49:60 GMT"},
        {784111777 + 24,            "Sunday, 06-Nov-94 08:49:61 GMT"},
        {784111777 + 25*24*60*60,   "Thu Nov 31 08:49:37 1994"},
        {784111777 + 26*24*60*60,   "Tue Nov 32 08:49:37 1994"},
        {784111777 + 16*60*60,      "Sun Nov  6 24:49:37 1994"},
        {784111777 + 60,            "Sun Nov  6 08:60:37 1994"},
        {784111777 + 23,            "Sun Nov  6 08:49:60 1994"},
        {784111777 + 24,            "Sun Nov  6 08:49:61 1994"},
        {0,0}
    };
    
    /* In these days, we run through all the characters and add
     * replace with an X, verifying we reject a bad characnter. */
    static const char *badchars[] = {
        "Sun, 06 Nov 1994 08:49:37 GMT\0\0",
        "Sunday, 06-Nov-94 08:49:37 GMT\0\0",
        "Sun Nov  6 08:49:37 1994\0\0",
        0
    };
    size_t i;
    char status[16];

    /* 
     * Run through all known bad strings
     */
    for (i=0; bads[i].string; i++) {
        const char *string = bads[i].string;
        time_t expected = bads[i].expected_result;
        test1(string, strlen(string), expected, status);
        fprintf(stdout, "[%s] 0x%09llx %s\n",
                status, (long long)expected, string);
    }

    /* 
     * Run through all bad numbers
     */
    for (i=0; badnums[i].string; i++) {
        const char *string = badnums[i].string;
        time_t expected = badnums[i].expected_result;
        test1(string, strlen(string), expected, status);
        fprintf(stdout, "[%s] 0x%09llx %s\n",
                status, (long long)expected, string);
    }

    /* 
     * Do the bad character tests
     * for all patterns
     */
    for (i=0; badchars[i]; i++) {
        const char *bad = badchars[i];
        size_t j;
        static const time_t expected = 784111777;

        /* for all mutations of the pattern */
        for (j=0; j<strlen(badchars[i])+1; j++) {
            char string[1024];

            memcpy(string, bad, strlen(bad)+2);
            string[j] = 'X';
            
            test1(string, strlen(string), expected, status);
            fprintf(stdout, "[%s] 0x%09llx %s\n",
                    status, (long long)expected, string);
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
        0x0f48656ff, /* Thu Dec 31 23:59:59 2099 */
        0x0f4865700,
        0x0f4865701,
        0x0f4d376c0,
        0x0f4d4c840, 
        -1
    };
    static const struct testcases_t others[] = {
        {0x58684680, "Sat, 31 Dec 2016 23:59:60 GMT"},
        {0,0}
    };
    size_t i;

    for (i=0; others[i].string; i++) {
        const char *bad = others[i].string;
        time_t expected = others[i].expected_result;
        int err;
        err = test_bad(bad, expected);
        if (err)
            return err;
    }


    /* RFC1123/IMF-time */
    for (i=0; goods[i] != -1; i++) {
        time_t expected = goods[i];
        struct tm *tm;
        size_t length;
        char status[16];

        tm = gmtime(&expected);
        length = strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", tm);
        if (length == 0) {
            fprintf(stderr, "[-] strftime() error\n");
            continue;
        }

        test1(buf, length, expected, status);
        fprintf(stdout, "[%s] 0x%09llx %s\n", status, (long long)expected, buf);

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

        test_bad(buf, good);
    }

    /* asctime() */
    for (i=0; goods[i] != -1; i++) {
        time_t good = goods[i];
        struct tm *tm;
        const char *buf2;
        

        tm = gmtime(&good);
        buf2 = asctime(tm);
        strcpy(buf, buf2);
        
        while (buf[0] && isspace(buf[strlen(buf)-1]&0xFF))
            buf[strlen(buf)-1] = '\0'; /*strip trailing whitespace*/
        

        test_bad(buf, good);
    }


    return 0; /* success */
}

/**
 * This runs through the contents of a file that should be in the same format
 * as the output this program produces. On input, it skips the "status" column
 * and reads the "expected" column.
 *
 * The intent is that regression tests then simply
 */
static int test_file(const char *filename) {
    char line[4096];
    FILE *fp;
    
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror(filename);
        return 1;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        time_t expected;

        /* remove trailing whitespace */
        while (line[0] && isspace(line[strlen(line)-1]))
            line[strlen(line)-1] = '\0'; /* trim trailing whitespace */

        /* skip comments */
        if (line[0] == '#' || line[0] == ';' || line[0] == '/') {
            printf("%s\n", line);
            continue;
        }
        
        /* skip empty lines */
        if (line[0] == '\0') {
            printf("\n");
            continue;
        }
        
        /* remove status column */
        while (line[0] && !isspace(line[0]))
            memmove(line, line+1, strlen(line));
        
        /* remove leading whitespace */
        while (line[0] && isspace(line[0]))
            memmove(line, line+1, strlen(line));
        
        expected = strtoul(line, 0, 0);

        /* remove expected column */
        while (line[0] && !isspace(line[0]))
            memmove(line, line+1, strlen(line));
        
        /* remove leading whitespace */
        while (line[0] && isspace(line[0]))
            memmove(line, line+1, strlen(line));
                
        {
            size_t string_length = strlen(line);
            size_t original_length = string_length;
            char status[16];
            
            
            test1(line, original_length, expected, status);
            fprintf(stdout, "[%s] 0x%09llx %s\n", status, (long long)expected, line);
        }

    }
    
    fclose(fp);
    return 0;
}

/* For LightHTTPD */
time_t log_epoch_secs;



int main(int argc, char *argv[]) {
    int i;
    int errs = 0;
    int test_count = 0;
    
    /* LightHTTPD init. It's got some weird optimization where it
     * doesn't call `time(0)` a lot, but instead sticks that value
     * in a global, and code references the global to avoid a
     * system call. */
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
        } else if (strcmp("--date", arg) == 0) {
            const char *string = argv[i+1];
            size_t length = strlen(string);
            char status[16];
            time_t result;
            
            
            result = parse_http_date(string, length);
            test1(string, strlen(string), result, status);
            fprintf(stdout, "[%s] 0x%09llx %s\n", status, (long long)result, string);
            i++;
            test_count++;
        } else if (strcmp("--file", arg) == 0) {
            const char *string = argv[i+1];
            errs += test_file(string);
            i++;
            test_count++;
        } else if (arg[0] != '-') {
            /* assume it's a file */
            errs += test_file(arg);
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
        fprintf(stderr, " --date <string>\n");
        fprintf(stderr, " --file <filename>\n");
    }

    if (errs)
        fprintf(stderr, "[-] date test %d errors\n", errs);
    return errs;
}

