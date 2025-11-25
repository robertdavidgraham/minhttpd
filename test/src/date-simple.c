#define _GNU_SOURCE     // for timegm() on glibc; on BSDs it's always available
/*
    Simplest HTTP Date: format parser
 
 This is the simplest parser for parsing the HTTP date that I could come
 up with.
 
 It uses the `strptime()`. This first appeared in SysV Unix in the 1980s,
 and was encorporated into FreeBSD 3.0 in 1998. It was standardized as
 part of POSIX in IEEE Std 1003.1-2001.
 
 It's not part of Windows, so when building this on Windows, an open-source
 version of the function is included with this file.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>


time_t simple_parse_date(const char *s)
{
    static const char *fmts[] = {
        "%a, %d %b %Y %H:%M:%S GMT",   /* IMF-fixdate, RFC1123, RFC822 */
        "%A, %d-%b-%y %H:%M:%S GMT",   /* RFC-850 NTTP */
        "%a %b %d %H:%M:%S %Y",        /* asctime() */
    };
    static const size_t fmts_count = sizeof(fmts)/sizeof(fmts[0]);
    struct tm tm;
    size_t i;
    
    for (i = 0; i < fmts_count; i++) {
        const char *fmt = fmts[i];
        const char *end;
        
        memset(&tm, 0, sizeof(tm));
        tm.tm_isdst = -1; /* no daylights saving time */

        end = strptime(s, fmt, &tm);
        
        if (end ==  NULL)
            continue; /* format invalid */
        if (*end != '\0')
            continue; /* didn't consume entire string */
        
        /* we have valid result */
        return timegm(&tm);
    }

    return (time_t)-1; /* fail */
}


