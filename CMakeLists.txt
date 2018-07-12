cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CONFIGURATION_TYPES Debug;Release)

 project(c_hash_cmp_log)
add_executable(c_hash_cmp_log
        main.cpp
		other.cpp
        pchheader.h
		main.h
		stdafx.h
        )

#add_executable (c_hash_cmp_log m2.cpp)

target_include_directories(c_hash_cmp_log
        PUBLIC
        C:/Users/baic/Downloads/DO_NOT_sync/ms-gsl/include)

target_include_directories(c_hash_cmp_log
        PRIVATE
        "include")


# include_directories("include")
#include_directories("${PROJECT_SOURCE_DIR}/include")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin1")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

# set(SOURCE_FILES
# supportFunctions.h
# supportFunctions.cpp
# main.h
# pchheader.h
# main.cpp
# )


target_link_libraries(c_hash_cmp_log stdc++fs ktmw32)