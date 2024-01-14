#include "pch.h"

#include "Log.h"
#if ENABLE_LOGGING

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Log::s_Logger;

void Log::Init() {
  std::vector<spdlog::sink_ptr> logSinks;
#if STDOUT_LOGGING
  logSinks.emplace_back(
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  logSinks.back()->set_pattern("%^[%T]: %v%$");
#endif

#if FILE_LOGGING
  logSinks.emplace_back(
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logging.log", true));
  logSinks.back()->set_pattern("[%T] [%l]: %v (%s)");
#endif

  s_Logger =
      std::make_shared<spdlog::logger>("GL", logSinks.begin(), logSinks.end());
  spdlog::register_logger(s_Logger);
  s_Logger->set_level(spdlog::level::trace);
  s_Logger->flush_on(spdlog::level::trace);
}
#endif
