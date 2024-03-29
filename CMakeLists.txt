cmake_minimum_required(VERSION 3.10)

set(PROJECT_N UWBLIB)
project(${PROJECT_N} VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/src/*.c)

# a macro that gets all of the header containing directories. 
MACRO(header_directories return_list includes_base_folder extention)
    FILE(GLOB_RECURSE new_list ${includes_base_folder}/*.${extention})
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()
# a macro that gets all of the header containing directories.
header_directories(INCLUDES ${PROJECT_SOURCE_DIR}/include/ h)


message(STATUS ${SRCS})
add_executable(${PROJECT_N} ${SRCS})

find_package(GSL REQUIRED)
target_link_libraries(${PROJECT_N} m GSL::gsl GSL::gslcblas)

target_include_directories(${PROJECT_N} PUBLIC include)
