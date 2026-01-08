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
    set(CURL_USE_LIBPSL OFF)

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
      URL https://github.com/curl/curl/releases/download/curl-8_18_0/curl-8.18.0.tar.xz
      URL_HASH
        SHA256=40df79166e74aa20149365e11ee4c798a46ad57c34e4f68fd13100e2c9a91946
      USES_TERMINAL_DOWNLOAD TRUE)
    FetchContent_MakeAvailable(curl)
    get_property(
      CURL_ALIAS_TARGET
      TARGET libcurl
      PROPERTY ALIASED_TARGET)
    if("${CURL_ALIAS_TARGET}" STREQUAL "")
      set_property(TARGET libcurl PROPERTY FOLDER "external")
    else()
      set_property(TARGET ${CURL_ALIAS_TARGET} PROPERTY FOLDER "external")
    endif()
    message(STATUS "Building libcurl using FetchContent")
  endif()
endif()
