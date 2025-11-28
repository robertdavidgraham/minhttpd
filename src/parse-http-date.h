#ifndef PARSE_HTTP_DATE_H
#define PARSE_HTTP_DATE_H
#include <limits.h>
#include <time.h>


/** 
 * Some states we external to the state-machien that tells us when it's done.
 */
enum {
    TEMP_INVALID = 0xFFFd,
    DATE_VALID = 0xFFFc,
    DATE_INVALID = 0xFFFf,
};

/** Some scratch pad state */
struct HttpDate {
    time_t timestamp;
    unsigned short year; /* 1970 through 2100 */
    unsigned char month; /* 1 through 12 */
    unsigned char day; /* 1 through 31 */
    unsigned char weekday; /* 0 through 6 */
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
    int is_until_crlf;
};

/**
 * Parses the contents of an HTTP "Date:" field (or others like 
 * "If-Modified-Since:". It strictly follows the three standard date
 * formats (RFC822, RFC850, and asctime). It is a "state-machine" parser
 * which means it can be called multiple times to progressively parse
 * fragments, without first needing to reassemble those fragments.
 * 
 * @param state
 *  Must be set to zero on the first call. Subsequence calls should pass
 *  in the return value from the previous call.
 * @param buf
 *  A fragment (or maybe the entire buffer) of input that we need to parse
 *  for the date.
 * @param length
 *  The length of the buffer on input, the number of bytes parsed on
 *  output.
 * @param result
 *  When a valid date has been parsed, this will be held by result->timestamp.
 *  Otherwise, this contains some intermediate parsing information.
 * @return
 *  DATE_VALID if we have successfully completed the task.
 *  DATE_INVALID if we have failed in parsing a date, but have otherwise
 *  reached the end of input.
 *  Any other value is an an opaque internal state that must be supplied
 *  to the next call of the function on the next fragment of input.
 */
unsigned parse_http_date_crlf(unsigned state, const void *buf, size_t *length, struct HttpDate *result);

time_t parse_http_date(const void *buf, size_t length);


/**
 * On startup, initialize this.
 */
void init_http_date_parser(void);


#endif /*PARSE_HTTP_DATE_H*/


