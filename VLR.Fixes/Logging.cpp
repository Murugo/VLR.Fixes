//#include <mutex>
//#include <sstream>

#include "pch.h"

#include "Logging.h"

namespace {

constexpr char kLogFilename[] = "VLR.Fixes.log.txt";

}  // namespace

FileLogger* FileLogger::instance_{ nullptr };
std::mutex FileLogger::mutex_;

FileLogger* FileLogger::Get()
{
    std::scoped_lock lock(mutex_);
    if (instance_ == nullptr)
    {
        instance_ = new FileLogger();
    }
    return instance_;
}

void FileLogger::Write(const std::string& entry, LogSeverity severity)
{
    std::scoped_lock lock(mutex_);

    if (!enabled_)
    {
        return;
    }
    if (!fstream_)
    {
        fstream_.reset(new std::ofstream(kLogFilename));
    }

    (*fstream_) << "[ ";
    switch (severity)
    {
    case LOG_INFO: (*fstream_) << "I "; break;
    case LOG_WARNING: (*fstream_) << "W "; break;
    case LOG_ERROR: (*fstream_) << "E "; break;
    default: break;
    }

    SYSTEMTIME time = {};
    GetLocalTime(&time);
    char time_buf[50];
    snprintf(time_buf, sizeof(time_buf), "%02hu-%02hu-%02hu %02hu:%02hu:%02hu", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

    (*fstream_) << time_buf << " ] " << entry << std::endl;
    fstream_->flush();
}

void FileLogger::SetEnabled(bool enabled)
{
    enabled_ = enabled;
}

std::ostringstream& LogEntry::Get()
{
    return os_;
}

LogEntry::~LogEntry()
{
    FileLogger::Get()->Write(os_.str(), severity_);
}
