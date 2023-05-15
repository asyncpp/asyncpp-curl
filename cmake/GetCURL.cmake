if(TARGET libcurl)
    message(STATUS "Using existing libcurl target.")
elseif(HUNTER_ENABLED)
    hunter_add_package(CURL)
    hunter_add_package(OpenSSL)
    find_package(CURL CONFIG REQUIRED)
    find_package(OpenSSL REQUIRED)
else()
    find_package(CURL)
    find_package(OpenSSL)

    if(NOT CURL_FOUND OR NOT OPENSSL_FOUND)
        unset(CURL_FOUND)
        unset(OPENSSL_FOUND)
    endif()

    if(NOT CURL_FOUND)
        # We only need HTTP (and HTTPS) support:
        set(HTTP_ONLY
            ON
            CACHE INTERNAL "" FORCE)
        set(BUILD_CURL_EXE
            OFF
            CACHE INTERNAL "" FORCE)
        set(BUILD_SHARED_LIBS
            OFF
            CACHE INTERNAL "" FORCE)
        set(BUILD_TESTING OFF)

        if(WIN32)
            set(CMAKE_USE_SCHANNEL
                ON
                CACHE INTERNAL "" FORCE)
        else()
            set(CMAKE_USE_OPENSSL
                ON
                CACHE INTERNAL "" FORCE)
        endif()

        include(FetchContent)
        FetchContent_Declare(
            curl
            URL https://github.com/curl/curl/releases/download/curl-7_80_0/curl-7.80.0.tar.xz
            URL_HASH
            SHA256=a132bd93188b938771135ac7c1f3ac1d3ce507c1fcbef8c471397639214ae2ab # the file

            # hash for
            # curl-7.80.0.tar.xz
            USES_TERMINAL_DOWNLOAD TRUE)
        FetchContent_MakeAvailable(curl)
        set_property(TARGET libcurl PROPERTY FOLDER "external")
        message(STATUS "Building libcurl using FetchContent")
    endif()
endif()