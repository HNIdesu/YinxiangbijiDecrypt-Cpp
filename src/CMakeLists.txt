add_executable(yinxiangbijidecrypt yinxiangbijidecrypt.cpp)
add_library(utility STATIC utility.cpp)
add_library(logger STATIC logger.cpp)
target_link_libraries(yinxiangbijidecrypt 
    xerces-c
    pthread
    crypto
    logger
    utility
    -ldl
)

