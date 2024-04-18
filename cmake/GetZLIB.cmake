if(TARGET ZLIB::ZLIB)
  message(STATUS "Using existing ZLIB target.")
else()
  find_package(ZLIB)

  if(NOT ZLIB_FOUND)
    set(ZLIB_BUILD_EXAMPLES
        OFF
        CACHE INTERNAL "" FORCE)

    include(FetchContent)
    FetchContent_Declare(
      zlib
      URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.xz
      URL_HASH
        SHA256=38ef96b8dfe510d42707d9c781877914792541133e1870841463bfa73f883e32
      USES_TERMINAL_DOWNLOAD TRUE)
    FetchContent_MakeAvailable(zlib)
    set_property(TARGET zlib PROPERTY FOLDER "external")
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
    message(STATUS "Building zlib using FetchContent")
  endif()
endif()
