cmake_minimum_required(VERSION 3.15) # Because we need
                                     # FetchContent_MakeAvailable
cmake_policy(VERSION 3.15)
if(POLICY CMP0135) # DOWNLOAD_EXTRACT_TIMESTAMP
  cmake_policy(SET CMP0135 OLD)
endif()

project(AsyncppCURL)

find_package(Threads REQUIRED)
if(HUNTER_ENABLED)
  hunter_add_package(CURL)
  find_package(CURL CONFIG REQUIRED)
else()
  find_package(CURL)
  if(NOT CURL)
    # We only need HTTP (and HTTPS) support:
    set(HTTP_ONLY ON CACHE INTERNAL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE INTERNAL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
    set(BUILD_TESTING OFF)
    set(CURL_CA_PATH "auto" CACHE INTERNAL "")
    set(CURL_CA_BUNDLE "auto" CACHE INTERNAL "")
    set(CMAKE_USE_OPENSSL ON CACHE INTERNAL "" FORCE)
    include(FetchContent)
    FetchContent_Declare(curl
                         URL                    https://github.com/curl/curl/releases/download/curl-7_80_0/curl-7.80.0.tar.xz
                         URL_HASH               SHA256=a132bd93188b938771135ac7c1f3ac1d3ce507c1fcbef8c471397639214ae2ab # the file hash for curl-7.80.0.tar.xz
                         USES_TERMINAL_DOWNLOAD TRUE)
    FetchContent_MakeAvailable(curl)
    set_property(TARGET libcurl PROPERTY FOLDER "external")
    message(STATUS "Building libcurl using FetchContent")
  endif()
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
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/sha1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/slist.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/tcp_client.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/uri.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/webclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/websocket.cpp)
target_link_libraries(asyncpp_curl PUBLIC asyncpp)
target_link_libraries(asyncpp_curl PUBLIC CURL::libcurl Threads::Threads)
target_include_directories(asyncpp_curl
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(asyncpp_curl PUBLIC cxx_std_20)
if(NOT HUNTER_ENABLED)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(asyncpp_curl PUBLIC OpenSSL::Crypto OpenSSL::SSL)
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
    ${CMAKE_CURRENT_SOURCE_DIR}/test/webclient.cpp)
  target_link_libraries(asyncpp_curl-test PRIVATE asyncpp_curl GTest::gtest
                                                  GTest::gtest_main Threads::Threads)

  if(ASYNCPP_WITH_ASAN)
    message(STATUS "Building with asan enabled")
    if(MSVC)
      target_compile_options(asyncpp_curl-test PRIVATE -fsanitize=address /Zi)
      target_compile_definitions(asyncpp_curl-test
                                PRIVATE _DISABLE_VECTOR_ANNOTATION)
      target_compile_definitions(asyncpp_curl-test
                                PRIVATE _DISABLE_STRING_ANNOTATION)
      target_link_libraries(asyncpp_curl-test PRIVATE libsancov.lib)
    else()
      target_compile_options(asyncpp_curl-test PRIVATE -fsanitize=address)
    endif()
  endif()

  gtest_discover_tests(asyncpp_curl-test)

  add_executable(asyncpp_curl-ws-autobahn
                 ${CMAKE_CURRENT_SOURCE_DIR}/test/ws-autobahn.cpp)
  target_link_libraries(asyncpp_curl-ws-autobahn PRIVATE asyncpp_curl
                                                         Threads::Threads)

  if(ASYNCPP_WITH_ASAN)
    target_compile_options(asyncpp_curl-ws-autobahn PRIVATE -fsanitize=address)
    target_link_libraries(asyncpp_curl-ws-autobahn PRIVATE asan)
    message(STATUS "Building with asan enabled")
    if(MSVC)
      target_compile_options(asyncpp_curl-ws-autobahn PRIVATE -fsanitize=address /Zi)
      target_compile_definitions(asyncpp_curl-ws-autobahn
                                 PRIVATE _DISABLE_VECTOR_ANNOTATION)
      target_compile_definitions(asyncpp_curl-ws-autobahn
                                 PRIVATE _DISABLE_STRING_ANNOTATION)
      target_link_libraries(asyncpp_curl-ws-autobahn PRIVATE libsancov.lib)
    else()
      target_compile_options(asyncpp_curl-ws-autobahn PRIVATE -fsanitize=address)
    endif()
  endif()
endif()
