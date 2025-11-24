#include <time.h>
#include <ctype.h>

time_t litespeed_parseHttpTime(const char *s, int len)
{
    static const unsigned int daytab[2][13] =
    {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
    };
    unsigned sec, min, hour, day, mon, year;
    char month[3] = { 0, 0, 0 };
    enum { D_START, D_END, D_MON, D_DAY, D_YEAR, D_HOUR, D_MIN, D_SEC };

    sec = 60;
    min = 60;
    hour = 24;
    day = 0;
    year = 0;
    {
        char ch;
        unsigned n;
        char flag;
        char state;
        char type;
        const char *end = s + len;
        type = 0;
        state = D_START;
        n = 0;
        flag = 1;
        for (ch = *s; (ch && state != D_END && s < end); ch = *s++)
        {
            switch (state)
            {
            case D_START:
                if (' ' == ch)
                {
                    state = D_MON;
                    n = 0;
                    type = 3;
                }
                else if (ch == ',') state = D_DAY;
                break;
            case D_MON:
                if ((ch == ' ') && (n == 0))
                    ;
                else if (isalpha(ch))
                {
                    if (n < 3) month[n++] = ch;
                }
                else
                {
                    if (n < 3)
                        return 0;
                    if ((1 == type) && (' ' != ch))
                        return 0;
                    if ((2 == type) && ('-' != ch))
                        return 0;
                    flag = 1;
                    state = (type == 3) ? D_DAY : D_YEAR;
                    n = 0;
                }
                break;
            case D_DAY:
                if (ch == ' ' && flag)
                    ;
                else if (isdigit(ch))
                {
                    flag = 0;
                    n = 10 * n + (ch - '0');
                }
                else
                {
                    if ((' ' != ch) && ('-' != ch))
                        return 0;
                    if (!type)
                    {
                        if (ch == ' ')
                            type = 1;
                        else if (ch == '-')
                            type = 2;
                    }
                    day = n;
                    n = 0;
                    flag = 1;
                    state = (type == 3) ? D_HOUR : D_MON;
                }
                break;
            case D_YEAR:
                if (ch == ' ' && flag)
                    ;
                else if (isdigit(ch))
                {
                    flag = 0 ;
                    year = 10 * year + (ch - '0');
                }
                else
                {
                    n = 0;
                    flag = 1;
                    state = (type == 3) ? D_END : D_HOUR;
                }
                break;
            case D_HOUR:
                if ((' ' == ch) && flag)
                    ;
                else if (isdigit(ch))
                {
                    n = 10 * n + (ch - '0');
                    flag = 0;
                }
                else
                {
                    if (ch != ':')
                        return 0;
                    hour = n;
                    n = 0;
                    state = D_MIN;
                }
                break;
            case D_MIN:
                if (isdigit(ch))
                    n = 10 * n + (ch - '0');
                else
                {
                    if (ch != ':')
                        return 0;
                    min = n;
                    n = 0;
                    state = D_SEC;
                }
                break;
            case D_SEC:
                if (isdigit(ch))
                    n = 10 * n + (ch - '0');
                else
                {
                    if (ch != ' ')
                        return 0;
                    sec = n;
                    n = 0;
                    flag = 1;
                    state = (type == 3) ? D_YEAR : D_END;
                }
                break;
            }
        }
        if ((state != D_END) && ((type != 3) || (state != D_YEAR)))
            return 0;
    }
    if (year <= 100)
        year += (year < 70) ? 2000 : 1900;
    if (sec >= 60 || min >= 60 || hour >= 24 || day == 0 || year < 1970)
        return 0;
    switch (month[0])
    {
    case 'A':
        mon = (month[1] == 'p') ? 4 : 8;
        break;
    case 'D':
        mon = 12;
        break;
    case 'F':
        mon = 2;
        break;
    case 'J':
        mon = (month[1] == 'a') ? 1 : ((month[2] == 'l') ? 7 : 6);
        break;
    case 'M':
        mon = (month[2] == 'r') ? 3 : 5;
        break;
    case 'N':
        mon = 11;
        break;
    case 'O':
        mon = 10;
        break;
    case 'S':
        mon = 9;
        break;
    default:
        return 0;
    }
    {
        const unsigned int *pDays = daytab[year % 4 == 0] + mon - 1;
        if (day > *(pDays + 1) - *pDays)
            return 0;
        --day;
        // leap day count is correct till DC 2100. so don't worry.
        return sec + 60L * (min + 60L * (hour + 24L * (
                                             day + *pDays +
                                             365L * (year - 1970L) + ((year - 1969L) >> 2))));
    }
}
