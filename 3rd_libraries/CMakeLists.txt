aux_source_directory(. DIR_SRCS)
FILE (GLOB DIR_INC "./include/*.h")
add_library(lib3d STATIC ${DIR_SRCS} ${DIR_INC})
target_include_directories(lib3d PUBLIC include)
