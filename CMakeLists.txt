cmake_minimum_required(VERSION 3.15) # Because we need
                                     # FetchContent_MakeAvailable
project(AsyncppCURL)

find_package(Threads REQUIRED)
if(HUNTER_ENABLED)
  hunter_add_package(CURL)
  find_package(CURL CONFIG REQUIRED)
else()
  find_package(CURL REQUIRED)
endif()

option(ASYNCPP_BUILD_TEST "Enable test builds" ON)
option(ASYNCPP_WITH_ASAN "Enable asan for test builds" ON)

if(TARGET asyncpp)
  message(STATUS "Using existing asyncpp target.")
else()
  message(STATUS "Missing asyncpp, using Fetch to import it.")
  include(cmake/Fetch_asyncpp.cmake)
endif()

add_library(
  asyncpp_curl
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/base64.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/exception.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/executor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/handle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/multi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/slist.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/tcp_client.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/uri.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/webclient.cpp)
target_link_libraries(asyncpp_curl PUBLIC asyncpp)
target_link_libraries(asyncpp_curl PRIVATE CURL::libcurl Threads::Threads)
target_include_directories(asyncpp_curl
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(asyncpp_curl PUBLIC cxx_std_20)
if(NOT HUNTER_ENABLED)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(asyncpp_curl PRIVATE OpenSSL::Crypto OpenSSL::SSL)
endif()

if(ASYNCPP_BUILD_TEST)
  enable_testing()
  include(GoogleTest)
  if(HUNTER_ENABLED)
    hunter_add_package(GTest)
    find_package(GTest CONFIG REQUIRED)
    add_library(GTest::GTest ALIAS GTest::gtest)
    add_library(GTest::Main ALIAS GTest::gtest_main)
  else()
    find_package(GTest REQUIRED)
  endif()

  add_executable(
    asyncpp_curl-test
    ${CMAKE_CURRENT_SOURCE_DIR}/test/base64.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/cookie.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/slist.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/tcp_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/uri.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/webclient.cpp)
  target_link_libraries(asyncpp_curl-test PRIVATE asyncpp_curl GTest::GTest
                                                  GTest::Main Threads::Threads)

  if(ASYNCPP_WITH_ASAN)
    message(STATUS "Building with asan enabled")
    target_compile_options(asyncpp_curl-test PRIVATE -fsanitize=address)
    target_link_libraries(asyncpp_curl-test PRIVATE asan)
  endif()

  gtest_discover_tests(asyncpp_curl-test)
endif()
