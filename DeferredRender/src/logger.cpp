#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

bool Logger::s_verbose = false;

// -------------------------------------------------------------------------
// Sub-logger registry - keeps named loggers alive and accessible.
// -------------------------------------------------------------------------
static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> s_loggers;
static std::vector<spdlog::sink_ptr> s_sinks; // shared across all loggers

// -------------------------------------------------------------------------
static std::shared_ptr<spdlog::logger> make_logger(
    const std::string& name,
    const std::vector<spdlog::sink_ptr>& sinks)
{
    auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace); // sinks control their own floors
    logger->flush_on(spdlog::level::warn);   // auto-flush on warnings and above
    spdlog::register_logger(logger);
    return logger;
}

// -------------------------------------------------------------------------
void Logger::init(bool verbose, const std::string& log_file)
{
    s_verbose = verbose;

    // --- stdout color sink ---
    // spdlog's stdout_color_sink_mt applies ANSI colors per level automatically.
    // The %^ ... %$ markers in the pattern define the colored region.
    // We intentionally do NOT call set_color() - the default level colors are
    // correct and the API signature differs between platforms (string_view on
    // POSIX, uint16_t on Windows), so avoiding it keeps the code portable.
    auto color_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    if (verbose) {
        // Full pattern: timestamp + level + logger name + file:line + message
        // %s = basename, %# = line number - these require SPDLOG_ACTIVE_LEVEL
        // to be set at compile time (done in CMakeLists.txt).
        color_sink->set_pattern("%^[%H:%M:%S.%e] [%n] (%s:%#)%$ %v");
        color_sink->set_level(spdlog::level::trace);
    }
    else {
        // Compact pattern: timestamp + level + logger name + message
        color_sink->set_pattern("%^[%H:%M:%S.%e] [%n]%$ %v");
        color_sink->set_level(spdlog::level::info);
    }

    s_sinks.push_back(color_sink);

    // --- optional file sink (always traces, no ANSI codes) ---
    if (!log_file.empty()) {
        try {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                log_file, /*truncate=*/true);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] (%s:%#) %v");
            file_sink->set_level(spdlog::level::trace);
            s_sinks.push_back(file_sink);
        }
        catch (const spdlog::spdlog_ex& ex) {
            // Logger isn't up yet - fprintf is the only option here
            fprintf(stderr, "[Logger] Could not open log file '%s': %s\n",
                log_file.c_str(), ex.what());
        }
    }

    // --- create built-in named loggers ---
    static const char* BUILTIN_LOGGERS[] = {
        "core", "vulkan", "render", "asset", "audio"
    };
    for (const char* name : BUILTIN_LOGGERS)
        s_loggers[name] = make_logger(name, s_sinks);

    spdlog::set_default_logger(s_loggers.at("core"));

    LOG_INFO("Logger initialised  verbose={}  sinks={}",
        verbose ? "on" : "off",
        log_file.empty() ? "stdout" : "stdout + " + log_file);
}

// -------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> Logger::get(const std::string& name)
{
    auto it = s_loggers.find(name);
    if (it != s_loggers.end())
        return it->second;

    // Unknown name - create on the fly using the shared sinks
    auto logger = make_logger(name, s_sinks);
    s_loggers[name] = logger;
    return logger;
}

// -------------------------------------------------------------------------
void Logger::set_level(LogLevel level)
{
    auto spd_level = static_cast<spdlog::level::level_enum>(level);
    for (auto& [name, logger] : s_loggers)
        logger->set_level(spd_level);
    for (auto& sink : s_sinks)
        sink->set_level(spd_level);
}