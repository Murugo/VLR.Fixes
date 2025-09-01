#pragma once

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

#define LOG(x) LogEntry(x).Get()

#define HEX(x) std::hex << std::uppercase << x << std::dec

enum LogSeverity {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

class FileLogger {
public:
    static FileLogger* Get();
    void Write(const std::string& entry, LogSeverity severity);

    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;

protected:
    FileLogger();

private:
    static std::mutex mutex_;
    static FileLogger* instance_;
    std::ofstream fstream_;
};

class LogEntry {
public:
    LogEntry(LogSeverity severity) : severity_(severity) {}
    ~LogEntry();

    std::ostringstream& Get();

    LogEntry(const LogEntry&) = delete;
    LogEntry& operator=(const LogEntry&) = delete;

private:
    std::ostringstream os_;
    LogSeverity severity_;
};
