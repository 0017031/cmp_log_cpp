cmake_minimum_required(VERSION 3.6)
project(c_hash_cmp_log)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

include_directories("include")
#include_directories("${PROJECT_SOURCE_DIR}/include")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin2")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

set(SOURCE_FILES
        include/args/args.hxx
        include/spdlog/details/async_log_helper.h
        include/spdlog/details/async_logger_impl.h
        include/spdlog/details/file_helper.h
        include/spdlog/details/log_msg.h
        include/spdlog/details/logger_impl.h
        include/spdlog/details/mpmc_bounded_q.h
        include/spdlog/details/null_mutex.h
        include/spdlog/details/os.h
        include/spdlog/details/pattern_formatter_impl.h
        include/spdlog/details/registry.h
        include/spdlog/details/spdlog_impl.h
        include/spdlog/fmt/bundled/format.h
        include/spdlog/fmt/bundled/ostream.h
        include/spdlog/fmt/bundled/printf.h
        include/spdlog/fmt/fmt.h
        include/spdlog/fmt/ostr.h
        include/spdlog/sinks/android_sink.h
        include/spdlog/sinks/ansicolor_sink.h
        include/spdlog/sinks/base_sink.h
        include/spdlog/sinks/dist_sink.h
        include/spdlog/sinks/file_sinks.h
        include/spdlog/sinks/msvc_sink.h
        include/spdlog/sinks/null_sink.h
        include/spdlog/sinks/ostream_sink.h
        include/spdlog/sinks/sink.h
        include/spdlog/sinks/stdout_sinks.h
        include/spdlog/sinks/syslog_sink.h
        include/spdlog/async_logger.h
        include/spdlog/common.h
        include/spdlog/formatter.h
        include/spdlog/logger.h
        include/spdlog/spdlog.h
        include/spdlog/tweakme.h
        supportFunctions.h
        supportFunctions.cpp
        main.h
        pchheader.h
        main.cpp
        )

add_executable(c_hash_cmp_log ${SOURCE_FILES})