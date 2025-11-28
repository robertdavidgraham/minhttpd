/*
    This module parses an HTTP date using a "state-machine parser".
 
    The purpose of this module is to demonstrate state-machine parsing
    more than solve the date parsing problem.

 Requirements:
    There are three formats, from RFC 822/1123/7231 (email),
    RFC 850 (usenet/nntp), and asctime(). They look like:

        Sun, 06 Nov 1994 08:49:37 GMT
        Sunday, 06-Nov-94 08:49:37 GMT
        Sun Nov  6 08:49:37 1994

    The official standards describe the formats using other notation, but I
    prefer to use a regex. The following would be a PCRE regex with "captures"
    that would parse these dates.

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

  Construction:
    This is a typie "state-machine" parser, meaning, that it parses the
*/


#include "parse-http-date.h"
#include "util-ahocorasick.h"
#include <ctype.h>
#include <time.h>

/*
 * These are "multi-pattern" matches. They scan forward in a state-machine
 * manner, one character at a time, to simulatneously search multiple
 * patterns at a time.
 */
static struct SMACK *wk;    /* Weekday names, like "Monday, ", "Mon ", and "Mon, " */
static struct SMACK *mon1;  /* Month names with space, "Jan " */
static struct SMACK *mon2;  /* Month names with dash, "Jan-" */

/*
 * Creates a multipattern matcher from a list of strings.
 */
static struct SMACK *
compile_multipatterns(const char *name, const char **strings) {
    struct SMACK *dfa;
    size_t i;

    dfa = smack_create(name, 0);
    for (i=0; strings[i]; i++)
        smack_add_pattern(dfa, strings[i], 0, i, SMACK_ANCHOR_BEGIN);
    smack_compile(dfa);

    return dfa;
}


/*
 * Called from main() at startup to initialize this matcher, which compiles
 * the strings int oa multi-pattern matcher.
 */
void
init_http_date_parser(void) {
    
    /* We determine which format the rest of the date string will be
     * by which row in this list matched. */
    /* (Sun|Mon|Tue|Wed|Thu|Fri|Sat),\s */
    /* (Sunday|Monday|Tuesday|Wednesday|Thursday|Friday|Saturday),\s */
    /* (Sun|Mon|Tue|Wed|Thu|Fri|Sat)\s */
    static const char *wknames[] = {
        "Sun, ", "Mon, ", "Tue, ", "Wed, ", "Thu, ", "Fri, ", "Sat, ",
        "Sunday, ", "Monday, ", "Tuesday, ", "Wednesday, ", "Thursday, ",
        "Friday, ", "Saturday, ",
        "Sun ", "Mon ", "Tue ", "Wed ", "Thu ", "Fri ", "Sat ",
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

    wk = compile_multipatterns("wk", wknames);
    mon1 = compile_multipatterns("mon1", mon1names);
    mon2 = compile_multipatterns("mon2", mon2names);
}

static int is_leap(long y) {
    return (!(y % 4) && (y % 100)) || !(y % 400);
}


/**
 * Convert a UTC calendar date/time to time_t (seconds since 1970-01-01 00:00:00 UTC).
 * year  : full year, e.g. 1970, 2025
 * month : 1..12
 * day   : 1..31
 * hour  : 0..23
 * min   : 0..59
 * sec   : 0..60 (will accept 60 for leap seconds; it will just roll forward)
 *
 * No loops, no mktime/timegm, no timezone or DST involved.
 */
static time_t
time_from_utc_components(unsigned year, unsigned month, unsigned day,
                         unsigned hour, unsigned min, unsigned sec)
{
    // Closed-form days-from-civil algorithm (Howard Hinnant style),
    // giving days since 1970-01-01 (the Unix epoch).
    unsigned y = year;
    unsigned m = (unsigned)month;
    unsigned d = (unsigned)day;
    unsigned era, yoe, doy, doe;
    unsigned days_since_epoch;
    int64_t seconds_in_day;
    int64_t total_seconds;

    // Move Jan/Feb into previous year as months 13/14 of y-1
    y -= (m <= 2);

    era  = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400; // [0, 399]
    doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
    doe = yoe * 365 + yoe / 4 - yoe / 100 + yoe / 400 + doy; // [0, 146096]

    // 719468 is the offset that makes 1970-01-01 -> day 0
    days_since_epoch = (era * 146097 + doe - 719468);

    // Now add seconds within the day.
    seconds_in_day = hour * 3600LL + min * 60LL + sec;

    // Combine to get seconds since the Unix epoch.
    total_seconds = days_since_epoch * 86400LL + seconds_in_day;

    return (time_t)total_seconds;
}

/**
 * Return weekday for a given Y/M/D.
 * 0 = Sunday, ... 6 = Saturday
 */
static int
get_day_of_week(int year, int month, int day)
{
    static const int mdays[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };
    int y;
    int m;
    int result;
    uint64_t days = 0; // Count days since 1970-01-01

    /* Add full years */
    for (y = 1970; y < year; y++) {
        days += 365 + (is_leap(y) ? 1 : 0);
    }

    /* Add full months of current year */
    for (m = 1; m < month; m++) {
        days += mdays[m - 1];
        if (m == 2 && is_leap(year))
            days += 1;  /* February 29 */
    }

    /* Add days of the current month */
    days += (day - 1);

    /* 1970-01-01 was a Thursday = weekday 4 */
    result = (4 + days) % 7;

    return result;
}


static time_t
calculate_result_time(const struct HttpDate *d) {
    return time_from_utc_components(
                    d->year, d->month, d->day,
                    d->hour, d->minute, d->second);
}

/**
 * Tests whether the day of the month is correct, whether it
 * has more than 30 days in November, for example (which is
 * invalid, as November has only 30 days). We can't do leap year
 * check for February, because we don't know the year at this
 * point.
 */
static int
is_valid_monthday(int month, int day) {
    switch (month) {
    case 1:  // Jan
    case 3:  // Mar
    case 5:  // May
    case 7:  // Jul
    case 8:  // Aug
    case 10: // Oct
    case 12: // Dec
        if (day > 31)
            return 0; /* invalid */
        break;

    case 4:  // Apr
    case 6:  // Jun
    case 9:  // Sep
    case 11: // Nov
        if (day > 30)
            return 0; /* invalid */
        break;

    case 2:  // Feb
        if (day > 29)
            return 0; /* invalid */
        break;

    default:
        return 0; /* invalid */
    }
    return 1;
}

/**
 * Check whether the "days" or "month" values are too large. We need
 * to know the year so we can specifically check whether February
 * has more than 28 days on non-leap years.
 */
static int
is_valid_date(int year, int month, int day) {
    if (year < 1970)
        return 0; /* invalid */

    if (!is_valid_monthday(month, day))
        return 0;
    if (month == 2) {
        if (is_leap(year)) {
            if (day > 29)
                return 0; /* invalid */
        } else {
            if (day > 28)
                return 0; /* invalid */
        }
    }
    return 1; /* true, numbers are valid */
}

/**
 * Transition to the invalid state. If we are parsing until CRLF,
 * then we transition to a "temporary" invalid state until we 
 * reach the end-of-line.
 */
static unsigned INVALID(struct HttpDate *result) {
    if (result->is_until_crlf)
        return TEMP_INVALID;
    else
        return DATE_INVALID;
}

/**
 * Don't transition, but continue in the current (outer) state, but
 * transition to the designated inner state.
 */
static unsigned CONTINUE(unsigned state, unsigned state_inner) {
    return state | (state_inner << 16);
}

/*
 * This is the core function that takes up most of this file. It parses
 * a single character, `c` and changes the state.
 */
static unsigned
parse_date_char(char c, unsigned state, struct HttpDate *result) {

    /*
     * This is the list of all the (outer) states in our state-machine.
     *
     * WARNING: DON'T CHANGE THE ORDER.
     * We often simply go to the next sequential state (state + 1), so
     * the order is important. For example, all the hour, minute, and
     * second states execute the same code that increments the state
     * going to the next one.
     */
    enum States {
        START=0,
        DAYNAME,

        /* Sun, 06 Nov 1994 08:49:37 GMT */
        DAY1NUM, MON1NAME, YEAR1, HOUR1, MIN1, SEC1, GMT1,

        /* Sunday, 06-Nov-94 08:49:37 GMT */
        DAY2NUM, MON2NAME, YEAR2, HOUR2, MIN2, SEC2, GMT2,

        /* Sun Nov  6 08:49:37 1994 */
        MON3NAME, DAY3NUM, HOUR3, MIN3, SEC3, YEAR3,

        VALID_CR, VALID_CRLF,
        
        _TEMP_INVALID = TEMP_INVALID,
        _DATE_VALID = DATE_VALID,
        _DATE_INVALID = DATE_INVALID,
    };
    size_t id;

    /* We pack two states into a single number, the outer state and
     * the inner state. We unpack them here, but when we return from
     * the function, we packet them back together again. The inner
     * state is for pattern-matching and parsing numbers */
    unsigned state_inner = (state>>16) & 0xFFFF;
    state = state & 0xFFFF;

    /* Carriage returns and line-feeds are never allowed except
     * at the end of the HTTP date field. Handle these characters
     * separately, REGARDLESS of which state we are processing. */
    if (c == '\r' && result->is_until_crlf) {
        if (state == VALID_CR)
            return VALID_CRLF;
        else
            return TEMP_INVALID;
    } else if (c == '\n' && result->is_until_crlf) {
        if (state == VALID_CRLF)
            return DATE_VALID;
        else
            return DATE_INVALID;
    }

    /*
     * This is the "state-machine". Each state represents one
     * field in the date.
     *
     * There is always a return from all the cases. It never
     * drops down afterwards.
     */
    switch (state) {
    case START:
        if (c == ' ' || c == '\t') {
            /* HTTP allows any amount of leading whitespace,
             * where whitespace is defined as either the
             * space character 0x20 or the horizontal-tab character
             * 0x09 */
            return CONTINUE(state, state_inner);
        } else {
            /* Start the numbers at zero */
            result->day = 0;
            result->month = 0;
            result->year = 0;
            result->hour = 0;
            result->minute = 0;
            result->second = 0;

            /* Start searching for the day-of-week */
            id = smack_search_next(wk, &state_inner, &c, 0, 1);
            if (id == SMACK_NOT_YET_FOUND)
                return CONTINUE(DAYNAME, state_inner);
            else if (id == SMACK_CANT_FIND)
                return INVALID(result);
        }
        break;

    /*[.xxxx........................]
      [Sun, 06 Nov 1994 08:49:37 GMT]
     */
    /*[.xxxxxxx......................]
      [Sunday, 06-Nov-94 08:49:37 GMT]
     */
    /*[.xxx....................]
      [Sun Nov  6 08:49:37 1994]
     */
    case DAYNAME:
        /* Keep matching the day-of-week field */
        id = smack_search_next(wk, &state_inner, &c, 0, 1);
        if (id == SMACK_NOT_YET_FOUND)
            return CONTINUE(state, state_inner);
        else if (id == SMACK_CANT_FIND)
            return INVALID(result);
        else {
            /* We found a day name */
            state_inner = 0;
            result->weekday = id % 7; /* 0=Sun, 1=Mon, ..., 6=Sat */

            /* We discover the format of the rest of the string by what
             * initially matched */
            switch (id/7) {
            case 0:
                return DAY1NUM; /*RFC822/RFC1123*/
            case 1:
                return DAY2NUM; /*RFC850*/
            case 2:
                return MON3NAME; /*asctime*/
            }
        }
        return INVALID(result);

    /*[.....xxx.....................]
      [Sun, 06 Nov 1994 08:49:37 GMT]
     */
    /*[........xxx...................]
      [Sunday, 06-Nov-94 08:49:37 GMT]
     */
    case DAY1NUM:
    case DAY2NUM:
        switch (state_inner) {
        case 0: /* first digit */
        case 1: /* second digit */
            if (!isdigit(c&0xFF))
                return INVALID(result);
            else {
                result->day = result->day * 10 + (c - '0');
                if (result->day > 31)
                    return INVALID(result);
                else
                    return CONTINUE(state, state_inner+1);
            }
            break;
        case 2: /* trailing space */
            if (state == DAY1NUM && c != ' ')
                return INVALID(result);
            else if (state == DAY2NUM && c != '-')
                return INVALID(result);
            else
                return state + 1; /* MON1NAME or MON2NAME */
            break;
        default:/* not possible */
            return INVALID(result);
        }
        break;

    /*[........xxx.............]
      [Sun Nov  6 08:49:37 1994]
     */
    case DAY3NUM:
        switch (state_inner) {
        case 0: /* first digit */
            if (c == ' ') /* may be space instead of digit */
                return CONTINUE(state, state_inner + 1);
            else if (!isdigit(c&0xFF))
                return INVALID(result);
            else {
                result->day = result->day * 10 + (c - '0');
                if (result->day > 3)
                    return INVALID(result);
                else
                    return CONTINUE(state, state_inner + 1);
            }
            break;
        case 1: /* second digit */
            if (!isdigit(c&0xFF))
                return INVALID(result);
            else {
                result->day = result->day * 10 + (c - '0');
                if (!is_valid_monthday(result->month, result->day))
                    return INVALID(result);
                else
                    return CONTINUE(state, state_inner+1);
            }
            break;
        case 2: /* trailing space */
            if (c != ' ')
                return INVALID(result);
            else
                return HOUR3;
            break;
        default:/* not possible */
            return INVALID(result);
        }
        break;

    /*[........xxxx.................]
      [Sun, 06 Nov 1994 08:49:37 GMT]
     */
    /*[....xxxx................]
      [Sun Nov  6 08:49:37 1994]
     */
    case MON1NAME:
    case MON3NAME:
        id = smack_search_next(/**/mon1/**/, &state_inner, &c, 0, 1);
        if (id == SMACK_NOT_YET_FOUND)
            return CONTINUE(state, state_inner);
        else if (id == SMACK_CANT_FIND || id >= 12)
            return INVALID(result);
        else {
            result->month = (unsigned char)id + 1; /* [1..12] */            
            return state + 1; /* YEAR1 or DAY3NUM */
        }
        break;

    /*[...........xxxx...............]
      [Sunday, 06-Nov-94 08:49:37 GMT]
     */
    case MON2NAME:
        id = smack_search_next(/**/mon2/**/, &state_inner, &c, 0, 1);
        if (id == SMACK_NOT_YET_FOUND)
            return CONTINUE(state, state_inner);
        if (id == SMACK_CANT_FIND || id >= 12)
            return INVALID(result);
    
        result->month = (unsigned char)id + 1; /* [1..12] */
            
        if (!is_valid_monthday(result->month, result->day))
            return INVALID(result);

        return YEAR2;
        break;

    /*[............xxxxx............]
      [Sun, 06 Nov 1994 08:49:37 GMT]
     */
    /*[....................xxxx]
      [Sun Nov  6 08:49:37 1994]
     */
    case YEAR1:
    case YEAR3:
        switch (state_inner) {
        case 0: /* first digit */
        case 1: /* second digit */
        case 2: /* third digit */
        case 3: /* fourth digit */
            if (c == ' ' && state_inner == 2)
                return INVALID(result); /*TODO: maybe handle 2-digit dates here */
            else if (!isdigit(c&0xFF))
                return INVALID(result);
            else {
                result->year = result->year * 10 + (c - '0');
                if (state_inner == 3 && state == YEAR3) {
                    result->timestamp = calculate_result_time(result);
                    if (result->is_until_crlf)
                        return VALID_CR;
                    else
                        return DATE_VALID;
                } else
                    return CONTINUE(state, state_inner + 1);
            }
            break;
        case 4: /* trailing space */
            if (!is_valid_date(result->year, result->month, result->day))
                return INVALID(result);
            if (c != ' ')
                return INVALID(result);
            return HOUR1;
            break;
        default:/* not possible */
            return INVALID(result);
        }
        break;

    /*[...............xxx............]
      [Sunday, 06-Nov-94 08:49:37 GMT]
     */
    case YEAR2:
        switch (state_inner) {
        case 0: /* first digit */
        case 1: /* second digit */
            if (!isdigit(c&0xFF))
                return INVALID(result);
            else {
                result->year = result->year * 10 + (c - '0');
                return CONTINUE(state, state_inner + 1);
            }
            break;
        case 2: /* trailing space */
            /* WARNING: we aren't following the typical algorithm to deal
             * with the Y2k issue here of everything above 70 being 1970<,
             * and everything below 69 is <2069. Instead, we are parsing
             * the day-of-week and from that determining whih century we
             * are in. This lasts until the 2200s. */
            if (result->weekday == get_day_of_week(result->year + 1900, result->month, result->day)) {
                result->year += 1900;
            } else if (result->weekday == get_day_of_week(result->year + 2000, result->month, result->day)) {
                result->year += 2000;
            } else if (result->weekday == get_day_of_week(result->year + 2100, result->month, result->day)) {
                result->year += 2100;
            } else
                result->year += 2000;
            
            /* We don't do this algorithm:
             if (result->year >= 70)
                result->year += 1900;
            else
                result->year += 2000;*/

            if (!is_valid_date(result->year, result->month, result->day))
                return INVALID(result);
            else if (c != ' ')
                return INVALID(result);
            else
                return HOUR2;
            break;
        default:/* not possible */
            return INVALID(result);
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
                return INVALID(result);
            else {
                result->hour = result->hour * 10 + (c - '0');
                if (result->hour >= 24)
                    return INVALID(result);
                else
                    return CONTINUE(state, state_inner + 1);
            }
            break;
        case 2: /* trailing colon*/
            if (c != ':')
                return INVALID(result);
            else
                return state + 1; /* go to MINx */
            break;
        default:/* not possible */
            return INVALID(result);
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
                return INVALID(result);
            else {
                result->minute = result->minute * 10 + (c - '0');
                if (result->minute > 59)
                    return INVALID(result);
                else
                    return CONTINUE(state, state_inner + 1);
            }
            break;
        case 2: /* trailing colon*/
            if (c != ':')
                return INVALID(result);
            else
                return state + 1; /* go to SECx */
            break;
        default:/* not possible */
            return INVALID(result);
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
                return INVALID(result);
            else {
                result->second = result->second * 10 + (c - '0');
                if (result->second > 60)
                    return INVALID(result);
                if (result->second == 60) {
                    /* leap seconds can only happen at midnight */
                    if (result->hour != 23 && result->minute != 59)
                        return INVALID(result);
                    else
                        result->second = 60;
                }
                return CONTINUE(state, state_inner + 1);
            }
            break;
        case 2: /* trailing colon*/
            if (c != ' ')
                return INVALID(result);
            else
                return state + 1; /* go to thing after SECx */
            break;
        default:/* not possible */
            return INVALID(result);
        }
        break;

    /* [..........................xxx]
     * [Sun, 06 Nov 1994 08:49:37 GMT]
     */
    case GMT1:
    case GMT2:
        if ("GMT"[state_inner] == c) {
            if (state_inner == 2) {
                result->timestamp = calculate_result_time(result);

                /* We are done parsing the date. Now, we may need to parse to the
                 * end of the line. */
                if (result->is_until_crlf)
                    return VALID_CR;
                else
                    return _DATE_VALID;
            } else
               return CONTINUE(state, state_inner + 1);     
        }
        else
            return INVALID(result);
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
            return VALID_CR;
        case '\r':
            return VALID_CRLF;
        default:
            return INVALID(result);
        }
        break;

    case VALID_CRLF:
        /* At this point, the only valid character is a line-feed
         * ending the line */
        if (c == '\n')
            return _DATE_VALID;
        else
            return INVALID(result);
        break;

    case _DATE_VALID:
        /* There shouldn't be anything after the date, but allow spaces */
        if (c == ' ' || c == '\t')
            return CONTINUE(state, 0);
        else
            /* illegal character after the date, so now invalid */
            return INVALID(result);
        break;
    case TEMP_INVALID:
        if (c == '\n')
            return DATE_INVALID;
        else
            return CONTINUE(state, 0);
        break;

    case _DATE_INVALID:
        break;
    }

    return state | (state_inner<<16);
}


/*
 *
 */
time_t
parse_http_date(    const void *buf,
                    size_t length
                    ) {
    const char *cbuf = (const char *)buf;
    size_t i;
    unsigned state = 0;
    struct HttpDate scratch[1] = {0};
    
    
    scratch->is_until_crlf = 0;

    for (i=0; i<length; i++) {
        char c = cbuf[i];

        state = parse_date_char(c, state, scratch);
    }
    if (state == DATE_VALID)
        return scratch->timestamp;
    else if (state == DATE_INVALID)
        return -1;
    else
        return -1;
}

/*
 *
 */
unsigned
parse_http_date_crlf(unsigned state,
                    const void *buf,
                    size_t *length,
                    struct HttpDate *result) {
    const char *cbuf = (const char *)buf;
    size_t i;
    result->is_until_crlf = 1;

    for (i=0; i<*length; i++) {
        char c = cbuf[i];

        state = parse_date_char(c, state, result);
        if (state == DATE_VALID || state == DATE_INVALID) {
            if (state == DATE_INVALID)
                result->timestamp = -1;
            if (state == DATE_VALID)
                i++;
            break;
        }
    }
    *length = i;
    return state;
}







