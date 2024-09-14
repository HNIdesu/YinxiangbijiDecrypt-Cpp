add_executable(yinxiangbijidecrypt yinxiangbijidecrypt.cpp)
add_library(utility STATIC utility.cpp)
add_library(logger STATIC logger.cpp)
add_library(path STATIC Path.cpp)
target_link_libraries(yinxiangbijidecrypt 
    xerces-c
    pthread
    crypto
    logger
    utility
    path
)

if (WIN32)
    target_link_libraries(yinxiangbijidecrypt crypt32 ws2_32)
elseif (UNIX)
    target_link_libraries(yinxiangbijidecrypt dl)
else ()
endif ()