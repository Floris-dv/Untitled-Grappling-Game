#include "pch.h"
#include "Log.h"

#pragma warning(push, 0)
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#pragma warning(pop)

std::shared_ptr<spdlog::logger> Log::s_Logger;

void Log::Init()
{
#if !DIST
	std::vector<spdlog::sink_ptr> logSinks;
	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logging.log", true));

	logSinks[0]->set_pattern("%^[%T]: %v%$");
	logSinks[1]->set_pattern("[%T] [%l]: %v (%s)");

	s_Logger = std::make_shared<spdlog::logger>("GL", begin(logSinks), end(logSinks));
	spdlog::register_logger(s_Logger);
	s_Logger->set_level(spdlog::level::trace);
	s_Logger->flush_on(spdlog::level::trace);
#endif
}