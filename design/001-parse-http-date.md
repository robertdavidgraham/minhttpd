Parsing HTTP date field
======

This document describes the concept of a "state-machine parser" by
describing how to parse the HTTP "Date:" field. This is specifically
used in client requests inside the "If-Modified-Since:" field.

Formats
----

There are three variations of this Date: format. While the server 
should only ever produce the first format, it must accept all
three formats from clients. This makes our parser fun an interesting.

Note that all end in GMT -- the only permissable timezone for HTTP.

1. IMF-fixdate (RFC 5322–style)

This is the official format, the only one we should actually use.
This comes originally from the RFC 822 "Date:" field in email messages,
and has since been updated to support four digit Y2K compliant dates.

The following is an example of how that looks:

	```
	Sun, 06 Nov 1994 08:49:37 GMT
	```

The following is a regex expression that will recognize this format. The
standards documents have different ways of expressing this, such as BNF
notation, but I think regex gets to the point easier.

	```regex
	^(Mon|Tue|Wed|Thu|Fri|Sat|Sun),\s
	([0-3][0-9])\s
	(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s
	[0-9]{4}\s
	([0-2][0-9]:[0-5][0-9]:[0-5][0-9])\sGMT$
	```

Of interest is whether we should support leap-seconds, meaning the
seconds field doesn't end at 59 but ends at 60, meaning
our regex should end at `[0-6][0-9]`.


2. RFC-850 date format (obsolete)

This comes from Usenet/NTTP messages. It's wildly obsolete and there's
a good question whether we should support it at all. We are going to,
but mostly becuase it'll make parsing interesting.

An example of this format looks like:

	```
	Sunday, 06-Nov-94 08:49:37 GMT
	```

The regex at parses this looks like:

	```regex
	^(Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday),\s
	([0-3][0-9])-(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)-[0-9]{2}\s
	([0-2][0-9]:[0-5][0-9]:[0-5][0-9])\sGMT$
	```


3. asctime() format (obsolete)

This is the default way that C programs product timestamps. It should never
appear in practice. We shouldn't even include it, but we do so because it
makes the parser interesting.

	```
	Sun Nov  6 08:49:37 1994
	```

The day of the month is two characters, in the case of days less than 10, the
leading character may be either a digit of 0 or an additional space.

	```regex
	^(Mon|Tue|Wed|Thu|Fri|Sat|Sun)\s
	(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s
	([ 0-9][0-9])\s
	([0-2][0-9]:[0-5][0-9]:[0-5][0-9])\s
	[0-9]{4}$
	```

We could write a **named capture** regex, like for PCRE, that would parse and
extract these values for us.

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

In principle, we could simply import the PCRE library and use this as our
parser.

But we are going to use a more straightforward coding method.


Leap Seconds
----

One question is whether the parser should support "leap seconds". Leap seconds happen
every few years for just a second. Often, they don't appear in timestamps at all.
Sometimes, they may be formatted in timestamps with the maximum number appearing
as `:60` in the seconds field.

One option is to ignore `:60` as invalid input. The other option is to treat it
the same as `:59`.


Case Insensitivity
----

HTTP dates are *case-insensitive*, for the days of the week, month names,
and the *GMT* at the end.


White-space
----

In HTTP fields in general, any amount of whitespace is allow before and
after the contents of the field:

```
Date: Sun, 06 Nov 1994 08:49:37 GMT\r\n
Date:    Sun, 06 Nov 1994 08:49:37 GMT    \r\n
Date : Sun, 06 Nov 1994 08:49:37 GMT\r\n

Only the space 0x20 and tab 0x09 characters are allowed as whitespace.

Bare carriage returns, vertical tabs, form feeds, or unicode whitespace
is not considered whitespace.

Inside the date contents itself, "Sun, 06 Nov 1994 08:49:37 GMT", they
must be the actual space character 0x20 and they must be the exact number
of spaces.


Other Parsers
----

Programming languages already have parsers that can be used to parse these
dates.

* C/POSIX strptime()
* Python strtotime()
* JavaScript Date
* Go time.Parse
* Rust chrono

In C, the POSIX function can be called with the following strings to 
parse the date:

```C
    static const char *fmts[] = {
        /* IMF-fixdate: Sun, 06 Nov 1994 08:49:37 GMT */
        "%a, %d %b %Y %H:%M:%S GMT",

        /* RFC 850: Sunday, 06-Nov-94 08:49:37 GMT */
        "%A, %d-%b-%y %H:%M:%S GMT",

        /* asctime(): Sun Nov  6 08:49:37 1994 */
        "%a %b %e %H:%M:%S %Y",
    };
```











