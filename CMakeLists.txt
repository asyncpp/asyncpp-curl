hunter_add_package(CURL)
find_package(CURL CONFIG REQUIRED)

add_library(asyncpp_curl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/exception.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/executor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/handle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/multi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/slist.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/curl/util.cpp
)
target_link_libraries(asyncpp_curl PRIVATE asyncpp CURL::libcurl Threads::Threads)
target_include_directories(asyncpp_curl
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)


if(BUILD_TEST)
    enable_testing()
    include(GoogleTest)
    if(HUNTER_ENABLED)
        hunter_add_package(GTest)
    endif()
    find_package(GTest REQUIRED)

    add_executable(asyncpp_curl-test
        ${CMAKE_CURRENT_SOURCE_DIR}/test/slist.cpp
    )
    target_link_libraries(asyncpp_curl-test PRIVATE asyncpp_curl GTest::GTest GTest::Main Threads::Threads)
    gtest_add_tests(TARGET asyncpp_curl-test)
endif()