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
* secure access to static files
* security hardening (containerizing)
* secure timeouts for slowloris type attacks
* IPv6 support


## Lession #1 - Parsers

The server is built using *state-machine* parsers. Most modern web servers
use this approach, like *nginx* or *LightHTTPD*. It's an integral part of
*asynchronous* server design.

This will be contrasted with *Apache*, which demonstrates the old style of
parsing.
