#ifndef LOGGER_H
#define LOGGER_H

// Pull in spdlog. Adjust this path to wherever your project keeps it.
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

// -------------------------------------------------------------------------
// Log level aliases — insulate call-sites from spdlog enum names so you
// could swap the back-end later without touching every file.
// -------------------------------------------------------------------------
enum class LogLevel {
    Trace   = SPDLOG_LEVEL_TRACE,
    Debug   = SPDLOG_LEVEL_DEBUG,
    Info    = SPDLOG_LEVEL_INFO,
    Warn    = SPDLOG_LEVEL_WARN,
    Error   = SPDLOG_LEVEL_ERROR,
    Fatal   = SPDLOG_LEVEL_CRITICAL,
    Off     = SPDLOG_LEVEL_OFF,
};

// -------------------------------------------------------------------------
// Logger — thin singleton wrapper.
// Call Logger::init() once at startup, then use the macros below.
// -------------------------------------------------------------------------
class Logger {
public:
    // Call before any log macro is used.
    // verbose=true drops the floor to Trace; false sets it to Info.
    // log_file: optional path — empty string disables file output.
    static void init(bool verbose = false,
                     const std::string& log_file = "");

    // Lower-level access if you need a named sub-logger (e.g. "audio").
    static std::shared_ptr<spdlog::logger> get(const std::string& name = "core");

    // Change level at runtime (e.g. after parsing --verbose flag).
    static void set_level(LogLevel level);

    static bool is_verbose() { return s_verbose; }

private:
    static bool s_verbose;
};

// -------------------------------------------------------------------------
// Macros — zero overhead when the level is below the active floor.
// Uses the "core" logger by default.  Pass a name as the first argument
// to route to a sub-logger: LOG_INFO_TO("render", "frame {}", n)
// -------------------------------------------------------------------------
#define LOG_TRACE(...)    SPDLOG_LOGGER_TRACE  (Logger::get(), __VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_LOGGER_DEBUG  (Logger::get(), __VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_LOGGER_INFO   (Logger::get(), __VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_LOGGER_WARN   (Logger::get(), __VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_LOGGER_ERROR  (Logger::get(), __VA_ARGS__)
#define LOG_FATAL(...)    SPDLOG_LOGGER_CRITICAL(Logger::get(), __VA_ARGS__)

// Named sub-logger variants
#define LOG_TRACE_TO(name, ...)  SPDLOG_LOGGER_TRACE  (Logger::get(name), __VA_ARGS__)
#define LOG_DEBUG_TO(name, ...)  SPDLOG_LOGGER_DEBUG  (Logger::get(name), __VA_ARGS__)
#define LOG_INFO_TO(name, ...)   SPDLOG_LOGGER_INFO   (Logger::get(name), __VA_ARGS__)
#define LOG_WARN_TO(name, ...)   SPDLOG_LOGGER_WARN   (Logger::get(name), __VA_ARGS__)
#define LOG_ERROR_TO(name, ...)  SPDLOG_LOGGER_ERROR  (Logger::get(name), __VA_ARGS__)
#define LOG_FATAL_TO(name, ...)  SPDLOG_LOGGER_CRITICAL(Logger::get(name), __VA_ARGS__)

// Verbose-only alias: compiles away to nothing when level >= Info.
// Use for per-frame or high-frequency messages you never want in prod.
#define LOG_VERBOSE(...) LOG_TRACE(__VA_ARGS__)

#endif