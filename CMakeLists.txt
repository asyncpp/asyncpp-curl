cmake_minimum_required(VERSION 3.15) # Because we need
                                     # FetchContent_MakeAvailable
cmake_policy(VERSION 3.15)
if(POLICY CMP0135) # DOWNLOAD_EXTRACT_TIMESTAMP
  cmake_policy(SET CMP0135 OLD)
endif()

project(AsyncppCURL)

find_package(Threads REQUIRED)

include(cmake/GetCURL.cmake)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/sha1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/slist.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/tcp_client.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/uri.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/version.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/webclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/websocket.cpp)
target_link_libraries(asyncpp_curl PUBLIC asyncpp)
target_link_libraries(asyncpp_curl PUBLIC CURL::libcurl Threads::Threads)
target_include_directories(asyncpp_curl
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(asyncpp_curl PUBLIC cxx_std_20)
if(NOT HUNTER_ENABLED AND NOT WIN32)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(asyncpp_curl PUBLIC OpenSSL::Crypto OpenSSL::SSL)
endif()
if(ASYNCPP_WITH_ASAN)
  message(STATUS "Building with asan enabled")
  if(MSVC)
    target_compile_options(asyncpp_curl PRIVATE -fsanitize=address /Zi)
    target_compile_definitions(asyncpp_curl PRIVATE _DISABLE_VECTOR_ANNOTATION)
    target_compile_definitions(asyncpp_curl PRIVATE _DISABLE_STRING_ANNOTATION)
    target_link_libraries(asyncpp_curl PRIVATE libsancov.lib)
  else()
    target_compile_options(asyncpp_curl PRIVATE -fsanitize=address)
    target_link_options(asyncpp_curl PRIVATE -fsanitize=address)
  endif()
endif()

if(ASYNCPP_BUILD_TEST)
  enable_testing()
  include(GoogleTest)
  if(HUNTER_ENABLED)
    hunter_add_package(GTest)
    find_package(GTest CONFIG REQUIRED)
  else()
    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG release-1.12.1)
    if(WIN32)
      set(gtest_force_shared_crt
          ON
          CACHE BOOL "" FORCE)
      set(BUILD_GMOCK
          OFF
          CACHE BOOL "" FORCE)
    endif()
    FetchContent_MakeAvailable(googletest)
  endif()

  add_executable(
    asyncpp_curl-test
    ${CMAKE_CURRENT_SOURCE_DIR}/test/base64.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/cookie.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/slist.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/tcp_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/uri.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/version.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/webclient.cpp)
  target_link_libraries(
    asyncpp_curl-test PRIVATE asyncpp_curl GTest::gtest GTest::gtest_main
                              Threads::Threads)

  if(ASYNCPP_WITH_ASAN)
    if(MSVC)
      target_compile_options(asyncpp_curl-test PRIVATE -fsanitize=address /Zi)
      target_compile_definitions(asyncpp_curl-test
                                 PRIVATE _DISABLE_VECTOR_ANNOTATION)
      target_compile_definitions(asyncpp_curl-test
                                 PRIVATE _DISABLE_STRING_ANNOTATION)
      target_link_libraries(asyncpp_curl-test PRIVATE libsancov.lib)
    else()
      target_compile_options(asyncpp_curl-test PRIVATE -fsanitize=address)
      target_link_options(asyncpp_curl-test PRIVATE -fsanitize=address)
    endif()
  endif()

  gtest_discover_tests(asyncpp_curl-test)

  add_executable(asyncpp_curl-ws-autobahn
                 ${CMAKE_CURRENT_SOURCE_DIR}/test/ws-autobahn.cpp)
  target_link_libraries(asyncpp_curl-ws-autobahn PRIVATE asyncpp_curl
                                                         Threads::Threads)

  if(ASYNCPP_WITH_ASAN)
    if(MSVC)
      target_compile_options(asyncpp_curl-ws-autobahn PRIVATE -fsanitize=address
                                                              /Zi)
      target_compile_definitions(asyncpp_curl-ws-autobahn
                                 PRIVATE _DISABLE_VECTOR_ANNOTATION)
      target_compile_definitions(asyncpp_curl-ws-autobahn
                                 PRIVATE _DISABLE_STRING_ANNOTATION)
      target_link_libraries(asyncpp_curl-ws-autobahn PRIVATE libsancov.lib)
    else()
      target_compile_options(asyncpp_curl-ws-autobahn
                             PRIVATE -fsanitize=address)
      target_link_options(asyncpp_curl-ws-autobahn PRIVATE -fsanitize=address)
    endif()
  endif()
endif()
