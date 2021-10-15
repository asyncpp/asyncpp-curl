include(FetchContent)

FetchContent_Declare(
    asyncpp
    GIT_REPOSITORY "git@gitlab.com:Thalhammer/asyncpp.git"
)
FetchContent_MakeAvailable(asyncpp)