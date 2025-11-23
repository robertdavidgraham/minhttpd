/*
    This module parses an HTTP date.

    There are three formats, from RFC 822 (email), RFC 850 (usenet/nntp),
    and asctime().

    The easiest way to describe their format, and how to parse them,
    is in "regex capture" format, supported by such things as PCRE.

```regex
^(?:
    # IMF-fixdate: Sun, 06 Nov 1994 08:49:37 GMT
    (?P<wk1>Mon|Tue|Wed|Thu|Fri|Sat|Sun),\s
    (?P<day1>[0-3][0-9])\s
    (?P<mon1>Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s
    (?P<year1>[0-9]{4})\s
    (?P<hour1>[0-2][0-9]):(?P<min1>[0-5][0-9]):(?P<sec1>[0-5][0-9])\sGMT
|
    # RFC-850: Sunday, 06-Nov-94 08:49:37 GMT
    (?P<wk2>Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday),\s
    (?P<day2>[0-3][0-9])-
    (?P<mon2>Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)-
    (?P<year2>[0-9]{2})\s
    (?P<hour2>[0-2][0-9]):(?P<min2>[0-5][0-9]):(?P<sec2>[0-5][0-9])\sGMT
|
    # asctime: Sun Nov  6 08:49:37 1994
    (?P<wk3>Mon|Tue|Wed|Thu|Fri|Sat|Sun)\s
    (?P<mon3>Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s
    (?P<day3>[ 0-9][0-9])\s
    (?P<hour3>[0-2][0-9]):(?P<min3>[0-5][0-9]):(?P<sec3>[0-5][0-9])\s
    (?P<year3>[0-9]{4})
)$
```
*/


#include "parse-http-date.h"
#include "util-ahocorasick.h"
#include <ctype.h>
#include <time.h>

static struct SMACK *wk;

static struct SMACK *mon1;
static struct SMACK *gmt1;

static struct SMACK *mon2;
static struct SMACK *gmt2;

static struct SMACK *mon3;

static struct SMACK *
_compile(const char *name, const char **strings) {
    struct SMACK *dfa;
    size_t i;

    dfa = smack_create(name, SMACK_CASE_INSENSITIVE);
    for (i=0; strings[i]; i++)
        smack_add_pattern(dfa, strings[i], 0, i, SMACK_ANCHOR_BEGIN);
    smack_compile(dfa);

    return dfa;
}


void
init_http_date_parser(void) {
    
    /* (Mon|Tue|Wed|Thu|Fri|Sat|Sun),\s */
    /* (Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday),\s */
    /* (Mon|Tue|Wed|Thu|Fri|Sat|Sun)\s */
    static const char *wknames[] = {
        "Mon, ", "Tue, ", "Wed, ", "Thu, ", "Fri, ", "Sat, ", "Sun, ",
        "Monday, ", "Tuesday, ", "Wednesday, ", "Thursday, ",
        "Friday, ", "Saturday, ", "Sunday, ",
        "Mon ", "Tue ", "Wed ", "Thu ", "Fri ", "Sat ", "Sun ",
        0
    };

    /* (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s */
    static const char *mon1names[] = {
        "Jan ", "Feb ", "Mar ", "Apr ", "May ", "Jun ",
        "Jul ", "Aug ", "Sep ", "Oct ", "Nov ", "Dec ",
        0
    };


    /* (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)-\s */
    static const char *mon2names[] = {
        "Jan-", "Feb-", "Mar-", "Apr-", "May-", "Jun-",
        "Jul-", "Aug-", "Sep-", "Oct-", "Nov-", "Dec-",
        0
    };


    wk = _compile("wk", wknames);
    mon1 = _compile("mon1", mon1names);
    mon2 = _compile("mon2", mon2names);
    mon3 = mon2;
}

/* Public domain implementation of timegm() from OpenBSD */
static int is_leap(long y) {
    return (!(y % 4) && (y % 100)) || !(y % 400);
}

static const int days_before_month[2][12] = {
    { 0,31,59,90,120,151,181,212,243,273,304,334 }, /* normal */
    { 0,31,60,91,121,152,182,213,244,274,305,335 }  /* leap   */
};

time_t portable_timegm(struct tm *tm)
{
    long year = tm->tm_year + 1900;
    long month = tm->tm_mon;
    long day = tm->tm_mday - 1;
    long days = 0;
    long y = 0;
    long long seconds = 0;

    /* Normalize month/year pair */
    if (month < 0 || month > 11) {
        year += month / 12;
        month %= 12;
        if (month < 0) {
            month += 12;
            year--;
        }
    }

    /* Days from 1970 to given year */
    for (y = 1970; y < year; y++)
        days += is_leap(y) ? 366 : 365;
    for (y = year; y < 1970; y++)
        days -= is_leap(y) ? 366 : 365;

    /* Add days before this month */
    days += days_before_month[is_leap(year)][month];

    /* Add days within the month */
    days += day;

    /* Add the time of day */
    seconds =
        days * 86400LL +
        tm->tm_hour * 3600LL +
        tm->tm_min  * 60LL   +
        tm->tm_sec * 1LL;

    return (time_t)seconds;
}

static time_t
_calculate_timestamp(const struct HttpDate *d) {
    struct tm tm = {0};
    time_t result = -1;

    /* Fill in UTC/GMT time values */
    tm.tm_year = d->year - 1900;/* Years since 1900 */
    tm.tm_mon  = d->month - 1;  /* Months since January (0 = Jan, 11 = Dec) */
    tm.tm_mday = d->day;        /* Day of month [1–31] */
    tm.tm_hour = d->hour;       /* Hours [0–23] */
    tm.tm_min  = d->minute;     /* Minutes [0–59] */
    tm.tm_sec  = d->second;     /* Seconds [0–60] (60 only for leap seconds) */
    tm.tm_isdst = 0;            /* UTC never uses DST */

    result = portable_timegm(&tm);

    return result;
}

static
_is_valid_date(int year, int month, int day) {
        switch (month) {
        case 1:  // Jan
        case 3:  // Mar
        case 5:  // May
        case 7:  // Jul
        case 8:  // Aug
        case 10: // Oct
        case 12: // Dec
            if (day > 31)
                return 0; /* false */
            break;

        case 4:  // Apr
        case 6:  // Jun
        case 9:  // Sep
        case 11: // Nov
            if (day > 31)
                return 0; /* false */
            break;

        case 2:  // Feb
            if (is_leap(year)) {
                if (day > 29)
                    return 0;
            } else {
                if (day > 28)
                    return 0;
            }
            break;

        default:
            return 0;
    }

    return 1; /* true, nubmers are valid */
}

unsigned 
_parse_http_date(char c, unsigned state, struct HttpDate *result) {
    enum {
        START=0,
        DAYNAME,

        /* Sun, 06 Nov 1994 08:49:37 GMT */
        DAY1NUM, MON1NAME, YEAR1, HOUR1, MIN1, SEC1, GMT1,

        /* Sunday, 06-Nov-94 08:49:37 GMT */
        DAY2NUM, MON2NAME, YEAR2, HOUR2, MIN2, SEC2, GMT2,

        /* Sun Nov  6 08:49:37 1994 */
        MON3NAME, DAY3NUM, HOUR3, MIN3, SEC3, YEAR3,

        VALID_CR, VALID_CRLF,

        _DATE_VALID = DATE_VALID,
        _DATE_INVALID = DATE_INVALID,
    };

    unsigned state_outer = state & 0xFFFF;
    unsigned state_inner = (state>>16) & 0xFFFF;
    size_t id;

    switch (state_outer) {
    case START:
        if (c == ' ' || c == '\t') {
            /* HTTP allows any amount of leading whitespace,
             * where whitespace is defined as either the
             * space character 0x20 or the horizontal-tab character
             * 0x09 */
            break;
        } else {

            /* Start the numbers at zero */
            result->day = 0;
            result->month = 0;
            result->year = 0;
            result->hour = 0;
            result->minute = 0;
            result->second = 0;

            /* Initialize searching */
            id = smack_search_next(wk, &state_inner, &c, 0, 1);
            if (id == SMACK_NOT_YET_FOUND)
                state_outer = DAYNAME;
            else if (id == SMACK_CANT_FIND)
                state_outer = _DATE_INVALID;
        }
        break;

    case DAYNAME:
        /* Keep matching the day field */
        id = smack_search_next(wk, &state_inner, &c, 0, 1);
        if (id == SMACK_NOT_YET_FOUND) {
            ;
        } else if (id == SMACK_CANT_FIND) {
            state_outer = _DATE_INVALID;
        } else {
            /* We found a day name */
            int pattern = id / 7; /* 0=RFC822, 1=RFC850, 2=asctime */

            state_inner = 0;
            result->weekday = id % 7; /* 0=Sun, 1=Mon, ..., 6=Sat */

            switch (id/7) {
            case 0: /*RFC822*/
                state_outer = DAY1NUM;
                break;
            case 1: /*RFC850*/
                state_outer = DAY2NUM;
                break;
            case 2:/*asctime*/
                state_outer = MON3NAME;
                break;
            default:/* not possible */
                state_outer = _DATE_INVALID;
                break;
            }
        }
        break;

    /* Sun, 06 Nov 1994 08:49:37 GMT 
      [     ^^^                     ]*/
    case DAY1NUM:
        switch (state_inner) {
        case 0: /* first digit */
        case 1: /* second digit */
            if (!isdigit(c&0xFF))
                state_outer = _DATE_INVALID;
            else {
                result->day = result->day * 10 + (c - '0');
                state_inner++;
                if (result->day > 31) {
                    state_outer = _DATE_INVALID;
                }
            }
            break;
        case 2: /* trailing space */
            if (c != ' ')
                state_outer = _DATE_INVALID;
            else {
                state_outer = MON1NAME;
                state_inner = 0;
            }
            break;
        default:/* not possible */
            state_outer = _DATE_INVALID;
        }
        break;

    /* Sun, 06 Nov 1994 08:49:37 GMT 
      [        ^^^^                 ]*/
    case MON1NAME:
        id = smack_search_next(mon1, &state_inner, &c, 0, 1);
        if (id == SMACK_NOT_YET_FOUND)
            ;
        else if (id == SMACK_CANT_FIND || id >= 12)
            state_outer = _DATE_INVALID;
        else {
            result->month = id + 1; /* [1..12] */            
            state_outer = YEAR1;
            state_inner = 0;
        }
        break;

    /* Sun, 06 Nov 1994 08:49:37 GMT 
      [            ^^^^^            ]*/
    case YEAR1:
        switch (state_inner) {
        case 0: /* first digit */
        case 1: /* second digit */
        case 2: /* third digit */
        case 3: /* fourth digit */
            if (c == ' ' && state_inner == 2)
                state_outer = _DATE_INVALID; /*TODO: maybe handle 2-digit dates here */
            else if (!isdigit(c&0xFF))
                state_outer = _DATE_INVALID;
            else {
                result->year = result->year * 10 + (c - '0');
                state_inner++;
            }
            break;
        case 4: /* trailing space */
            if (!_is_valid_date(result->year, result->month, result->day))
                state_outer = _DATE_INVALID;
            else if (c != ' ')
                state_outer = _DATE_INVALID;
            else {
                state_outer = HOUR1;
                state_inner = 0;
            }
            break;
        default:/* not possible */
            state_outer = _DATE_INVALID;
        }
        break;

    /* 08:49:37
      [^^^     ]*/
    case HOUR1:
    case HOUR2:
    case HOUR3:
        switch (state_inner) {
        case 0: /* first digit */
        case 1: /* second digit */
            if (!isdigit(c&0xFF))
                state_outer = _DATE_INVALID;
            else {
                result->hour = result->hour * 10 + (c - '0');
                state_inner++;
            }
            break;
        case 2: /* trailing colon*/
            if (c != ':')
                state_outer = _DATE_INVALID;
            else {
                state_outer ++; /* go to MINx */
                state_inner = 0;
            }
            break;
        default:/* not possible */
            state_outer = _DATE_INVALID;
        }
        break;

    /* 08:49:37
      [   ^^^  ]*/
    case MIN1:
    case MIN2:
    case MIN3:
        switch (state_inner) {
        case 0: /* first digit */
        case 1: /* second digit */
            if (!isdigit(c&0xFF))
                state_outer = _DATE_INVALID;
            else {
                result->minute = result->minute * 10 + (c - '0');
                state_inner++;
            }
            break;
        case 2: /* trailing colon*/
            if (c != ':')
                state_outer = _DATE_INVALID;
            else {
                state_outer ++; /* go to SECx */
                state_inner = 0;
            }
            break;
        default:/* not possible */
            state_outer = _DATE_INVALID;
        }
        break;

    /* 08:49:37
      [      ^^^]*/
    case SEC1:
    case SEC2:
    case SEC3:
        switch (state_inner) {
        case 0: /* first digit */
        case 1: /* second digit */
            if (!isdigit(c&0xFF))
                state_outer = _DATE_INVALID;
            else {
                result->second = result->second * 10 + (c - '0');
                state_inner++;
            }
            break;
        case 2: /* trailing colon*/
            if (c != ' ')
                state_outer = _DATE_INVALID;
            else {
                state_outer ++; /* go to thing after SECx */
                state_inner = 0;
            }
            break;
        default:/* not possible */
            state_outer = _DATE_INVALID;
        }
        break;

    /* [..........................XXX]
     * "Sun, 06 Nov 1994 08:49:37 GMT"
     * [..........................XXX]*/
    case GMT1:
    case GMT2:
        if ("GMT"[state_inner] != c)
            state_outer = _DATE_INVALID;
        else if (++state_inner == 3) {
            /* We are done parsing the date. Now, we may need to parse to the
             * end of the line. */
            if (result->is_until_crlf) {
                state_outer = VALID_CR;
                state_inner = 0;
            } else {
                state_outer = _DATE_VALID;
                state_inner = 0;
                result->timestamp = _calculate_timestamp(result);
            }
        }
        break;

    case DAY2NUM:
    case MON2NAME:
    case YEAR2:
        break;

    case MON3NAME:
    case DAY3NUM:
    case YEAR3:
        break;

    case VALID_CR:
        /* There can be any amount of trailing whitespace, consisting of 
         * either space ' ' 0x20 or tab '\t' 0x09.
         * If a Carriage-Return CR '\r' 0x0D is seen, it must immediately
         * be followed by a line-feed.
         * Anything else is an error.
         */
        switch (c) {
        case ' ':
        case '\t':
            state_outer = VALID_CR;
            break;
        case '\r':
            state_outer = VALID_CRLF;
            break;
        default:
            state_outer = _DATE_INVALID;
            break;
        }
        break;

    case VALID_CRLF:
        /* At this point, the only valid character is a line-feed
         * ending the line */
        if (c == '\n') {
            state_outer = _DATE_VALID;
        } else {
            state_outer = _DATE_INVALID;
        }
        break;

    case _DATE_VALID:
        /* There shouldn't be anything after the date, but allow spaces */
        if (c == ' ' || c == '\t')
            break;
        else {
            /* illegal character after the date, so now invalid */
            state_outer = _DATE_INVALID;
        }
        break;
    case _DATE_INVALID:
        break;
    }

    return state_outer | (state_inner<<16);
}


unsigned 
parse_http_date(    unsigned state, 
                    const void *buf, 
                    size_t *length, 
                    struct HttpDate *result, 
                    int is_until_crlf) {
    const char *cbuf = (const char *)buf;
    size_t i;
    result->is_until_crlf = is_until_crlf;

    for (i=0; i<*length; i++) {
        char c = cbuf[i];

        state = _parse_http_date(c, state, result);
        if (state == DATE_VALID || state == DATE_INVALID) {
            break;
        }
    }
    *length = i;
    return state;
}



