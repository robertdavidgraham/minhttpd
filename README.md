minhttpd -- a minimal HTTP server
======

This is a minimal HTTP server designed for teaching (advanced) concepts
in network programming, parsers, scalability, and cybersecurity.

It's also functional and useful, something you can put on the public
Internet to host content. That is the point, teaching the more advanced
concepts needed in order to make something safe to put on the public
Internet, outside the firewall, where hackers can attack it.

A list of concepts its trying to teach:

* an asynchronous server using poll() or epoll()
* state-machine parsers
* proper SSL support
* secure URL parsing
* secure HTTP parsing to avoid pipelining/smuggling
* secure access to static files
* security hardening (containerizing)
* secure timeouts for slowloris type attacks
* IPv6 support

## Status

Currently [2025-12-01] the project only contains a state-machine parser
for HTTP `Date:` fields. This parser is finished, and I'm now creating
a test corpus to test it.

I'm especially looking at *parser differentials*,
adding to the test suite the `Date:` parsers from popular web servers like 
Apache, Nginx, and LiteSpeed. In general, the other webservers are very
permissive, allowing timestamps that go far outside the spec. Conversely,
LiteSpeed does not support one of the three standard formats allowed
in the `Date:` field, the so-called `asctime()` format.

## Layout of the project

A simple call to `make` will make everything, sticking the main program
in `bin`, and unit test programs in `test/bin`.

The source code is located in `src`, the test programs' source code
is located in `test/src`.

Everything test related is located under the `test` subdirectory, including
code but also data files containing test cases.

Calling `make test` will run all the unit and regression tests to validate
the product works.

But there are additional test programs for looking at other issues, like
benchmarking for performance and testing "parser differentials" against
snippets of code from other projects.

## Lession #1 - Parsers

The server is built using *state-machine* parsers. Most modern web servers
use this approach, like *nginx* or *LightHTTPD*. It's an integral part of
*asynchronous* server design.

This will be contrasted with *Apache*, which demonstrates the old style of
parsing.
