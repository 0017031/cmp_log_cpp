//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

// spdlog main header file.
// see example.cpp for usage example

#pragma once

#include <spdlog/tweakme.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>

#include <memory>
#include <functional>
#include <chrono>
#include <string>

namespace spdlog
{

//register logger
//void register_logger(std::shared_ptr<logger> logger);
void register_logger(std::shared_ptr<spdlog::logger> logger);

// Return an existing logger or nullptr if a logger with such name doesn't exist.
// Examples:
//
// spdlog::get("mylog")->info("Hello");
// auto logger = spdlog::get("mylog");
// logger.info("This is another message" , x, y, z);
// logger.info() << "This is another message" << x << y << z;
std::shared_ptr<logger> get(const std::string& name);

//
// Set global formatting
// example: spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e %l : %v");
//
void set_pattern(const std::string& format_string);
void set_formatter(formatter_ptr f);

//
// Set global logging level for
//
void set_level(level::level_enum log_level);

//
// Set global error handler
//
void set_error_handler(log_err_handler);

//
// Turn on async mode (off by default) and set the queue size for each async_logger.
// effective only for loggers created after this call.
// queue_size: size of queue (must be power of 2):
//    Each logger will pre-allocate a dedicated queue with queue_size entries upon construction.
//
// async_overflow_policy (optional, block_retry by default):
//    async_overflow_policy::block_retry - if queue is full, block until queue has room for the new log entry.
//    async_overflow_policy::discard_log_msg - never block and discard any new messages when queue  overflows.
//
// worker_warmup_cb (optional):
//     callback function that will be called in worker thread upon start (can be used to init stuff like thread affinity)
//
// worker_teardown_cb (optional):
//     callback function that will be called in worker thread upon exit
//
void set_async_mode(size_t queue_size, const async_overflow_policy overflow_policy = async_overflow_policy::block_retry, const std::function<void()>& worker_warmup_cb = nullptr, const std::chrono::milliseconds& flush_interval_ms = std::chrono::milliseconds::zero(), const std::function<void()>& worker_teardown_cb = nullptr);

// Turn off async mode
void set_sync_mode();


//
// Create and register multi/single basic file logger
//
std::shared_ptr<logger> basic_logger_mt(const std::string& logger_name, const filename_t& filename,bool force_flush = false);
std::shared_ptr<logger> basic_logger_st(const std::string& logger_name, const filename_t& filename, bool force_flush = false);

//
// Create and register multi/single threaded rotating file logger
//
std::shared_ptr<logger> rotating_logger_mt(const std::string& logger_name, const filename_t& filename, size_t max_file_size, size_t max_files, bool force_flush = false);
std::shared_ptr<logger> rotating_logger_st(const std::string& logger_name, const filename_t& filename, size_t max_file_size, size_t max_files, bool force_flush = false);

//
// Create file logger which creates new file on the given time (default in  midnight):
//
std::shared_ptr<logger> daily_logger_mt(const std::string& logger_name, const filename_t& filename, int hour=0, int minute=0, bool force_flush = false);
std::shared_ptr<logger> daily_logger_st(const std::string& logger_name, const filename_t& filename, int hour=0, int minute=0, bool force_flush = false);

//
// Create and register stdout/stderr loggers
//
std::shared_ptr<logger> stdout_logger_mt(const std::string& logger_name, bool color = false);
std::shared_ptr<logger> stdout_logger_st(const std::string& logger_name, bool color = false);
std::shared_ptr<logger> stderr_logger_mt(const std::string& logger_name, bool color = false);
std::shared_ptr<logger> stderr_logger_st(const std::string& logger_name, bool color = false);


//
// Create and register a syslog logger
//
#if defined(__linux__) || defined(__APPLE__)
std::shared_ptr<logger> syslog_logger(const std::string& logger_name, const std::string& ident = "", int syslog_option = 0);
#endif


// Create and register a logger a single sink
std::shared_ptr<logger> create(const std::string& logger_name, const sink_ptr& sink);

// Create and register a logger with multiple sinks
std::shared_ptr<logger> create(const std::string& logger_name, sinks_init_list sinks);
template<class It>
std::shared_ptr<logger> create(const std::string& logger_name, const It& sinks_begin, const It& sinks_end);


// Create and register a logger with templated sink type
// Example: spdlog::create<daily_file_sink_st>("mylog", "dailylog_filename", "txt");
template <typename Sink, typename... Args>
std::shared_ptr<logger> create(const std::string& logger_name, Args...);


// Register the given logger with the given name
void register_logger(std::shared_ptr<logger> logger);

// Drop the reference to the given logger
void drop(const std::string &name);

// Drop all references
void drop_all();


///////////////////////////////////////////////////////////////////////////////
//
// Trace & Debug can be switched on/off at compile time for zero cost debug statements.
// Uncomment SPDLOG_DEBUG_ON/SPDLOG_TRACE_ON in teakme.h to enable.
// SPDLOG_TRACE(..) will also print current file and line.
//
// Example:
// spdlog::set_level(spdlog::level::trace);
// SPDLOG_TRACE(my_logger, "some trace message");
// SPDLOG_TRACE(my_logger, "another trace message {} {}", 1, 2);
// SPDLOG_DEBUG(my_logger, "some debug message {} {}", 3, 4);
///////////////////////////////////////////////////////////////////////////////

#ifdef SPDLOG_TRACE_ON
#define SPDLOG_STR_H(x) #x
#define SPDLOG_STR_HELPER(x) SPDLOG_STR_H(x)
#define SPDLOG_TRACE(logger, ...) logger->trace("[" __FILE__ " line #" SPDLOG_STR_HELPER(__LINE__) "] " __VA_ARGS__)
#else
#define SPDLOG_TRACE(logger, ...)
#endif

#ifdef SPDLOG_DEBUG_ON
#define SPDLOG_DEBUG(logger, ...) logger->debug(__VA_ARGS__)
#else
#define SPDLOG_DEBUG(logger, ...)
#endif


}


#include <spdlog/details/spdlog_impl.h>
