# Async++ CURL library
This library provides a c++ wrapper around libcurl supporting async usage with c++20 coroutines.
It is an addition to async++ which provides general coroutine tasks and support classes.

Tested and supported compilers:
* Clang 12
* Clang 14

### Provided classes
* `slist` is a wrapper around curl slist's used for e.g. headers. Provides a stl container like interface
* `executor` is used for running a curl multi loop in an extra thread and providing a dispatcher interface for use with `defer`
* `handle` is a wrapper around a curl easy handle
* `multi` is a wrapper around a curl multi handle
* `base64` and `base64url` provides base64 encode and decode helpers
* `cookie` provides cookie handling and parsing
* `uri` provides URI parsing and building
* `http_request` and `http_response` provide a simplified interface to `handle` for doing normal HTTP transfers