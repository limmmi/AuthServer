#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

// 日志系统封装类
class Logger {
public:
    // 初始化日志系统
    // logPath: 日志文件路径
    // level: 日志级别 (trace, debug, info, warn, error, critical)
    static bool Initialize(const std::string& logPath = "./logs/auth_server.log",
                           const std::string& level = "info");
    
    // 关闭日志系统
    static void Shutdown();
    
    // 获取日志器
    static std::shared_ptr<spdlog::logger> GetLogger();
    
    // 便捷宏
    #define LOG_TRACE(...)    Logger::GetLogger()->trace(__VA_ARGS__)
    #define LOG_DEBUG(...)    Logger::GetLogger()->debug(__VA_ARGS__)
    #define LOG_INFO(...)     Logger::GetLogger()->info(__VA_ARGS__)
    #define LOG_WARN(...)     Logger::GetLogger()->warn(__VA_ARGS__)
    #define LOG_ERROR(...)    Logger::GetLogger()->error(__VA_ARGS__)
    #define LOG_CRITICAL(...) Logger::GetLogger()->critical(__VA_ARGS__)

private:
    static std::shared_ptr<spdlog::logger> logger_;
    static bool initialized_;
};

#endif // LOGGER_H
