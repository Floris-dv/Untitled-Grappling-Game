#pragma once
#include "Settings.h"

#if !DIST
// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#ifndef OVERLOAD
#define OVERLOAD 1
#endif

class Log
{
public:

	static void Init();

	static std::shared_ptr<spdlog::logger>& GetLogger() { return s_Logger; }
private:
	static std::shared_ptr<spdlog::logger> s_Logger;
};

#if OVERLOAD
template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}
#endif

// Logger macros, prefix to avoid name collisions, Neon Grappler
#define NG_TRACE(...)         ::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::trace, __VA_ARGS__)
#define NG_INFO(...)          ::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::info, __VA_ARGS__)
#define NG_WARN(...)          ::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::warn, __VA_ARGS__)
#define NG_ERROR(...)         ::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::err, __VA_ARGS__)
#define NG_CRITICAL(...)      ::Log::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::critical, __VA_ARGS__)

#else

#define NG_TRACE(...) (void)0
#define NG_INFO(...) (void)0
#define NG_WARN(...) (void)0
#define NG_ERROR(...) (void)0
#define NG_CRITICAL(...) (void)0

#endif

#define NG_CATCH_ALL(s) catch (const std::exception& e) { NG_ERROR("{} {}", s, e.what()); } \
						catch (const std::string& e) { NG_ERROR("{} {}", s, e); } \
						(void)0
