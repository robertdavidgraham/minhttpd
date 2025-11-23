minhttpd -- a minimal HTTP server
======

This is a minimal HTTP server designed for teaching purposes, to
teach important concepts in network programming. It's also functional
and useful, something you could put on the Internet to securely host
content. This is part of the point: it's teaching the more complicated
concepts you need to learn in order to build software that can be exposed
to the horrors of the public Internet, outside the firewall.

A list of concepts its trying to teach:

* an asynchronous server using poll() or epoll()
* state-machine parsers
* SSL support
* secure URL parsing
* secure access to static files
* security hardening
* secure timeouts for slowloris type attacks
* IPv6 support

