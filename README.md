# Async++ CURL library

[![License Badge](https://img.shields.io/github/license/asyncpp/asyncpp-curl)](https://github.com/asyncpp/asyncpp-curl/blob/master/LICENSE)
[![Stars Badge](https://img.shields.io/github/stars/asyncpp/asyncpp-curl)](https://github.com/asyncpp/asyncpp-curl/stargazers)

This library provides a c++ wrapper around libcurl supporting async usage with c++20 coroutines.
It is an addition to [async++](https://github.com/asyncpp/asyncpp) which provides general coroutine tasks and support classes.

Tested and supported compilers:
| Ubuntu 20.04                                                          | Ubuntu 22.04                                                          |
|-----------------------------------------------------------------------|-----------------------------------------------------------------------|
| [![ubuntu-2004_clang-10][img_ubuntu-2004_clang-10]][Compiler-Support] | [![ubuntu-2204_clang-12][img_ubuntu-2204_clang-12]][Compiler-Support] |
| [![ubuntu-2004_clang-11][img_ubuntu-2004_clang-11]][Compiler-Support] | [![ubuntu-2204_clang-13][img_ubuntu-2204_clang-13]][Compiler-Support] |
| [![ubuntu-2004_clang-12][img_ubuntu-2004_clang-12]][Compiler-Support] | [![ubuntu-2204_clang-14][img_ubuntu-2204_clang-14]][Compiler-Support] |
| [![ubuntu-2004_gcc-10][img_ubuntu-2004_gcc-10]][Compiler-Support]     | [![ubuntu-2204_gcc-10][img_ubuntu-2204_gcc-10]][Compiler-Support]     |
|                                                                       | [![ubuntu-2204_gcc-11][img_ubuntu-2204_gcc-11]][Compiler-Support]     |


[img_ubuntu-2004_clang-10]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2004_clang-10/shields.json
[img_ubuntu-2004_clang-11]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2004_clang-11/shields.json
[img_ubuntu-2004_clang-12]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2004_clang-12/shields.json
[img_ubuntu-2004_gcc-10]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2004_gcc-10/shields.json
[img_ubuntu-2204_clang-12]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2204_clang-12/shields.json
[img_ubuntu-2204_clang-13]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2204_clang-13/shields.json
[img_ubuntu-2204_clang-14]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2204_clang-14/shields.json
[img_ubuntu-2204_gcc-10]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2204_gcc-10/shields.json
[img_ubuntu-2204_gcc-11]: https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/asyncpp/asyncpp-curl/badges/compiler/ubuntu-2204_gcc-11/shields.json
[Compiler-Support]: https://github.com/asyncpp/asyncpp-curl/actions/workflows/compiler-support.yml

### Provided classes
* `slist` is a wrapper around curl slist's used for e.g. headers. Provides a stl container like interface
* `executor` is used for running a curl multi loop in an extra thread and providing a dispatcher interface for use with `defer`
* `handle` is a wrapper around a curl easy handle
* `multi` is a wrapper around a curl multi handle
* `base64` and `base64url` provides base64 encode and decode helpers
* `cookie` provides cookie handling and parsing
* `uri` provides URI parsing and building
* `http_request` and `http_response` provide a simplified interface to `handle` for doing normal HTTP transfers
