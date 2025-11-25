Parsing HTTP date field
======

The "If-Modified-Since:" field contains a date when the client last
retrieved and cached the content, meaning, the server can skip it if
there's been no change. It's a relative minor feature a minimal server
could skip, and it's relative easy to parse using standard methods.

We implement it in order to demonstrate the *state-machine parser* idea.


## What is a state-machine parser

A state-machine parser parses the content one byte at a time. In other
words, it would look like:

```c
    for (i=0; i<length && state != S_END; i++)
        state = parse_date(state, buf[i], &result);
```

There is a light of theory on *state-machines* and *finite-automata* which
may be helpful in understanding this.

The practical feature that's important is this integrates with *asynchronous*
servers. These servers, like *nginx* and *LightHTTPD* have better *scalability*
that old servers like *Apache*.

Whereas Apache first buffers an HTTP request header (in a 64k buffer) then
parses it, asynchronous servers parse bytes as they arrive. This gets rid
of the overhead of buffering.

The code above works either way. Maybe the entire HTTP request arrived as a
single packet, or maybe it arrived as a series of fragments. The parser itself
doesn't know the difference, because it only sees one character at a time,
sequentially.

##Requirements

There is only one format that should be generated, but three formats that
should be accepted. These three formats are:

```
    Sun, 06 Nov 1994 08:49:37 GMT
    Sunday, 06-Nov-94 08:49:37 GMT
    Sun Nov  6 08:49:37 1994
```

These are RFC822, RFC850, and asctime() formats respectively.

They are:
    * Always in GMT timezone
    * case-sensitive
    * no extra whitespace
    * two-digit numbers have two digits, where numbers smaller
      than 10 use a 0 in front to pad the number. The third format
      may optionally be padded with a space instead of a 0, either
      must be supported.
    * The strings for day-of-week and month are the fixed set of
      english names.
    * Leap seconds should be allowed, where the last seconds field
      is allowed to be :60, but must be rounded down to :59 in 
      interpretation. It shouldn't be produced.

### Format #1 - RFC-822, RFC-1123, RFC-5322

This is the format from the age-old RFC822 for email `Date:` fields,
except it uses a 4 digit year number rather than 2 digit. It is specified
in a lot of RFCs, so take your pick.

The following is an example of how that looks:

	```
	Sun, 06 Nov 1994 08:49:37 GMT
	```

The RFCs use BNF notation to specify it. I find it easier to use a *regex*
string. The following is a PCRE regex that can parse it.

	```regex
	^(Mon|Tue|Wed|Thu|Fri|Sat|Sun),\s
	([0-3][0-9])\s
	(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s
	[0-9]{4}\s
	([0-2][0-9]:[0-5][0-9]:[0-6][0-9])\sGMT$
	```
This could be extended with regex "capture-groups" and actually be used to
parse the date, especially when using a DFA-style regex.

Some things to note:
    * always using the English names
    * case-sensitive
    * always GMT, not UTC, and not any timezone
    * always 2-digit numbers, not a single digit for numbers smaller
      than 10 (and 4-digit for the year, of course)


### Format #2 - RFC-850 date format (obsolete)

This comes from Usenet/NTTP messages. 

There's a good question whether we should support it at all. We should follow
the anti-Postal law of making the parser as conservative as possible, even
removing features allowed by the standard.

However, we support it because it makes the state-machine parser more
interesting.

An example of this format looks like:

	```
	Sunday, 06-Nov-94 08:49:37 GMT
	```

The regex at parses this looks like:

	```regex
	^(Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday),\s
	([0-3][0-9])-(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)-[0-9]{2}\s
	([0-2][0-9]:[0-5][0-9]:[0-6][0-9])\sGMT$
	```

Since this is a 2-digit year, we need to know the cutoff. As specified
in RFC-2616 ¤3.3.1 and RFC-9110 ¤5.6.7, it matches the `time_t` Unix epoch,
where dates start at 1970: values 70-99 are considered part of
the 1900s, where nubmers 00-69 are considered part of the 20th century. Since
our parser rejects dates before 1970s, this makes sense.

Some things to note:
    * always using the English names
    * case-sensitive
    * always GMT, not UTC, and not any timezone
    * always 2-digit numbers, not a single digit for numbers smaller
      than 10 (and 4-digit for the year, of course)
    * hyphens between date components
    * day of week full spelled out instead of abbreviated


### Format #3 - asctime() format (obsolete)

This is the default way that C programs produces timestamps. It should never
appear in practice. We shouldn't even include it, but we do so because it
makes the parser interesting.

	```
	Sun Nov  6 08:49:37 1994
	```

The day of the month is two characters, in the case of days less than 10, the
leading character may be either a digit of 0 or an additional space. When
the space is used, it means there are two spaces in a row, one between fields,
and one to replace a digit.

	```regex
	^(Mon|Tue|Wed|Thu|Fri|Sat|Sun)\s
	(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s
	([ 0-9][0-9])\s
	([0-2][0-9]:[0-5][0-9]:[0-6][0-9])\s
	[0-9]{4}$
	```
 
 Some things to note:
    * always using the English names
    * case-sensitive
    * always GMT, not UTC, and not any timezone
    * allows day to be one digit (padded with space instead of 0), but
      time components are still always 2-digits
    * no commas
    * still assumed GMT, even though GMT isn't included
   
### All Formats

We could write a *named capture* regex, like for PCRE, that would parse and
extract these values for us.

```regex
^(?:
    # IMF-fixdate: Sun, 06 Nov 1994 08:49:37 GMT
    (?P<wk1>Mon|Tue|Wed|Thu|Fri|Sat|Sun),\s
    (?P<day1>[0-3][0-9])\s
    (?P<mon1>Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s
    (?P<year1>[0-9]{4})\s
    (?P<hour1>[0-2][0-9]):(?P<min1>[0-5][0-9]):(?P<sec1>[0-6][0-9])\sGMT
|
    # RFC-850: Sunday, 06-Nov-94 08:49:37 GMT
    (?P<wk2>Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday),\s
    (?P<day2>[0-3][0-9])-
    (?P<mon2>Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)-
    (?P<year2>[0-9]{2})\s
    (?P<hour2>[0-2][0-9]):(?P<min2>[0-5][0-9]):(?P<sec2>[0-6][0-9])\sGMT
|
    # asctime: Sun Nov  6 08:49:37 1994
    (?P<wk3>Mon|Tue|Wed|Thu|Fri|Sat|Sun)\s
    (?P<mon3>Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s
    (?P<day3>[ 0-9][0-9])\s
    (?P<hour3>[0-2][0-9]):(?P<min3>[0-5][0-9]):(?P<sec3>[0-6][0-9])\s
    (?P<year3>[0-9]{4})
)$
```

In principle, we could simply import the PCRE library and use this as our
parser.

But we are going to use a more straightforward coding method.

Another way of saying this is that the date formats are a *regular* language.
Anything that is *regular* like this, in the Chomsky hierarchy is by definition
something that can be parsed with a *state-machine*. The converse isn't true,
we'll use the "state-machine parsers" concept as the backbone for 
"push-down automata" and "context-senstive" parsers. Practically, it means
stashing data in temporary *stacks* and *tables* while we do what's
largley still just walking through a state-machine.


### Leap Seconds

One question is whether the parser should support "leap seconds".

The earth speeds up and slows down for various reasons. As a consequence,
the maintainers of such things add or remove a second from the UTC time,
the official time.

The last time this happened was

    ```
    Sat, 31 Dec 2016 23:59:60 UTC
    ```

The had announced they were going to do this on July 6, 2016, giving people
around 6 months to prepare computers for the change.

But, they've officially announced they are going to discontinue the practice
in 2035 because of the problems it causes to computers, such as the long
discussion in this document for something that'll likely never happen.

It's possible 2016 will have been the last leap second added. They were added
because the Earth's rotation had slightly slowed. In more recent years, though
it's slightly sped up (by milliseconds per day). Nobody is quite sure why,
maybe seismic activity, or maybe ice caps melting due to global warming
(seriously).

We can only see leap-seconds live, at the moment they are added. That's because
once they are added, we shift all old timestamps by a second. Technically, all
file timestamps from before the 2016 are off by at least a second. Files created
before the end of 1972 are off by a total of 27 seconds. But it's all still
consistent because we have no independent time to compare this too. It gets
really confusing, are the files older or newer than reported? (Think it through
-- I see then as seconds older, but maybe I'm no thinking it the correct way).

The upshot is that we have a requirement for our parser that may never appear
in the real world. This whole mess will end in 2035, in ten years, and because
the Earth has slightly sped up, we'll probably only see leap seconds removed
rather than added, so never see :60 at the end of a timestamp. (If they remove
a second, then 12:59:59 simply happens twice).

We can certainly implement the code and make sure it works, such as adding
`Sat, 31 Dec 2016 23:59:60 GMT` to our test suite. But what should you do?
You could ignore this requirement, but in 7 years have a pacemaker crash
because you decided not to add the code?


### Case Insensitivity


HTTP dates are *case-sensitive*, for the days of the week, month names,
and the *GMT* at the end.

It's a defficult requirement because nothing explicitly says this. However,
when they give the BNF format, implicitly the tokens are case-sensitive,
unless otherwise stated, and they never state otherwise.

HTTP field names are case-*insensitive*, thus, `Date:`, `DATE:`, `date:`,
and `dAtE:` are all valid.


### White-space

Inside the dates themselves, the space character is fixed. It's only
ever the space ' ' '\s' 0x20 character that's allowed, not any other
whitespace character. Also, it's just one the one space that's allowed
at each position, though in the *asctime()* format, two side-by-side
positions may have a space character.

However, there maybe whitespace around this field. HTTP fields
may be padded on either side by whitespace. For example, the
following are all valid.

```
Date: Sun, 06 Nov 1994 08:49:37 GMT\r\n
Date:    Sun, 06 Nov 1994 08:49:37 GMT    \r\n
Date : Sun, 06 Nov 1994 08:49:37 GMT\r\n

Only the space ' ' 0x20 and tab '\t\ 0x09 characters are allowed as 
whitespace in HTTP fields. There are no carriage-returns '\r', vertical-tabs,
form-feeds, unicode whitespace, and so on.

### Line-ending CRLF

Our parser will not onoy parse the contents of the field, but to the
end of the HTTP header line. Therefore, HTTP header rules apply.

In HTTP headers, the lines are ended with CRLF, a carriage-return plus 
line-feed pair. It's the only allowed ending. A bare LF is not enough,
it must be prefixed with a CR. A CR may not appear anywhere on the line
except right before the LF.

Thus, whenever our parser encounters a CR, it will immediately transition
to a state where it starts terminating the line, expecting an LF next.
Likewise, a LF is an error, unless it's prefixed by a CR.

## What other servers allow

We are applying the anti-Postel Principle, being as conservative as
possible. We may be even more conservative than the standard, rejecting
some things the standard allows. We certainly try to reject everything
the standard doesn't allow.

But in practice, a lot of competing projects are more lax, having implemented
the pro-Postel Principle of being lenient. Some software may depend upon that.
Therefore, to properly interoperate, we should firstly try to figure out
what popular competing projects support, even if in the near term we don't
plan on implementing those features, just in case in the future we have to.

Some features are:
    * allowing UTC instead of GMT
    * allowing +0000 instead of GMT
    * allowing the "GMT" field to be blank
    * case insenstive timezone, "gmt"
    * missing weekday, like `06 Nov 1994 08:49:37 GMT`
    * extra spaces between tokens
    * in asctime(), accept only a single space between the month
      and single digit day, so `Nov 6` instead of `Nov  6`.

We already allow:
    * allow 0 padded asctime() day, so `06` instead of ` 6`.


## Test suite

The project contains a `test` directory where we do all the tests.

It builds a `date-test` binary that unit/regression tests our "Date:"
parser independently from the rest of the HTTP server. That's sorta
the definition for a "unit" test -- a proper "unit" is something properly
modularized such that it can run as a stand-alone program or library.

It includes files from other projects' unit-tests containing samples
and what the correct value they are parsed to. It also contains test
cases that should be rejected, invalid.

The test program contains parsers from other projects in order to show
*parser-differentials*, how we parse differently than others do. Mostly,
this demonstrates that our parser is more conservative, rejecting timestamps
that others accept.

One optional test goes through the entire 32-bit space from 
from 1970-01-01 00:00:00 to Sunday, February 7, 2106, at 06:28:15 GMT,
formatting each time in all three formats, then parsing them to make sure
the parser works.

Finally, we have options for benchmarking, so that we can test the speed
of each of the different parsers. Apache's parser has a comment that it's
faster than the simpler alternative, so we want to see that in action,
whether it is, and whether it's faster than this state-machine parser.


## Other parsers

This project includes test code from other projects that similarly parse
the date field, so we can calculate *parse-differentials*. The code
from Apache (`apr-util`) is nicely modularized, so we can easily
extract it from that project and include it as a test program in our 
code.

For example, the Apache code ignores the day name, allowing any text
in that position, whereas we parse it and validate it.
This is demonstrated in a bunch of test cases.

In C, the POSIX function `strptime()` can be called with the following
format strings for each format of the date strings to parse the date.

```C
    static const char *fmts[] = {
        "%a, %d %b %Y %H:%M:%S GMT",
        "%A, %d-%b-%y %H:%M:%S GMT",
        "%a %b %e %H:%M:%S %Y",
    };
```

This can also be done with `sscanf()`.

We again include test cases that show the parser differential
between what this function will accept and what our code will
accept.

Note that this is also a demonstration of how we couldv'e implemented
this functionality much simpler: simply call `strptime()` instead of
going through this complicated effort of writing our own parser.
But the point is to teach writing state-machine parsers rather than
creating the simplest implementation.


Programming languages already have parsers that can be used to parse these
dates. You can probably send all three date formats to all of these languages.

* C/POSIX strptime()
* Python strtotime()
* JavaScript Date
* Go time.Parse
* Rust chrono

