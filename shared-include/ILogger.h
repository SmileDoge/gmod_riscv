#pragma once

#include <string>

enum class ILogType
{
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_DEBUG,

	WHITE,
};

class ILogger
{
public:
	virtual ~ILogger() = default;

	virtual void LogString(ILogType type, const std::string& string) = 0;

	virtual void LogInfo(const char* format, ...) = 0;
	virtual void LogWarn(const char* format, ...) = 0;
	virtual void LogError(const char* format, ...) = 0;
	virtual void LogDebug(const char* format, ...) = 0;

	virtual void LogDebugWithLine(int line, const std::string& file, const char* format, ...) = 0;
};

//extern ILogger* g_Logger;

#define RV__IFLOGGERPRESENTED(expr) do { if (g_Logger) { expr } } while (0)

#define RV_INFO(fmt, ...) RV__IFLOGGERPRESENTED(g_Logger->LogInfo(fmt __VA_OPT__(, ) __VA_ARGS__);)

#define RV_INFO(fmt, ...) RV__IFLOGGERPRESENTED(g_Logger->LogInfo(fmt __VA_OPT__(, ) __VA_ARGS__);)
#define RV_WARN(fmt, ...) RV__IFLOGGERPRESENTED(g_Logger->LogWarn(fmt __VA_OPT__(, ) __VA_ARGS__);)
#define RV_ERROR(fmt, ...) RV__IFLOGGERPRESENTED(g_Logger->LogError(fmt __VA_OPT__(, ) __VA_ARGS__);)

#ifdef _DEBUG

#define RV_DEBUG(fmt, ...) RV__IFLOGGERPRESENTED(g_Logger->LogDebug(fmt __VA_OPT__(, ) __VA_ARGS__);)
#define RV_DEBUG_LINE(fmt, ...) RV__IFLOGGERPRESENTED(g_Logger->LogDebugWithLine(__LINE__, __FUNCTION__, fmt __VA_OPT__(, ) __VA_ARGS__);)

#else

#define RV_DEBUG(fmt, ...) ((void)0)
#define RV_DEBUG_LINE(fmt, ...) ((void)0)

#endif