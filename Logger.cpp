#include "Logger.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>
#include <sys/stat.h>

std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;
bool Logger::initialized_ = false;

bool Logger::Initialize(const std::string& logPath, const std::string& level) {
    if (initialized_) {
        return true;
    }
    
    try {
        // 创建日志目录
        size_t lastSlash = logPath.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            std::string dir = logPath.substr(0, lastSlash);
            mkdir(dir.c_str(), 0755);
        }
        
        // 创建多目标日志器（控制台 + 文件）
        std::vector<spdlog::sink_ptr> sinks;
        
        // 控制台输出（带颜色）
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(spdlog::level::info);
        sinks.push_back(consoleSink);
        
        // 文件输出（轮转，最大10MB，保留3个备份）
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logPath, 10 * 1024 * 1024, 3);
        fileSink->set_level(spdlog::level::debug);
        sinks.push_back(fileSink);
        
        // 创建日志器
        logger_ = std::make_shared<spdlog::logger>("auth_server", sinks.begin(), sinks.end());
        
        // 设置日志级别
        logger_->set_level(spdlog::level::from_str(level));
        
        // 设置格式: [时间] [级别] 消息
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        
        // 设置立即刷新（关键日志）
        logger_->flush_on(spdlog::level::warn);
        
        initialized_ = true;
        LOG_INFO("Logger initialized, log file: {}", logPath);
        return true;
        
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        return false;
    }
}

void Logger::Shutdown() {
    if (initialized_ && logger_) {
        LOG_INFO("Logger shutting down");
        logger_->flush();
        spdlog::shutdown();
        initialized_ = false;
    }
}

std::shared_ptr<spdlog::logger> Logger::GetLogger() {
    if (!initialized_ || !logger_) {
        // 返回默认日志器（控制台）
        return spdlog::default_logger();
    }
    return logger_;
}
