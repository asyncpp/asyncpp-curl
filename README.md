# Async++ CURL library
This library provides a c++ wrapper around libcurl supporting async usage with c++20 coroutines.

Tested and supported compilers:
* Clang 12

**WARNING**: Neither this library nor compiler support for coroutines is what I would consider production ready. I do use it in some services where uptime is not important and have not found any significant issues, but you should probably make sure you have solid testing and don't value reliability too much.

### Provided classes
* `slist` Wrapper around curl slist's used for e.g. headers. Provides a stl container like interface
* `executor` Class running a curl multi loop in an extra thread and providing a dispatcher interface for use with `defer`
* `handle` Wrapper around a curl easy handle
* `multi` Wrapper around a curl multi handle
* `util` Some utility functions for base64 and url encoding