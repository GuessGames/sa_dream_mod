#pragma once

#include <chrono>
#include <ctime>

class logger
{
private:
    static void log(const char* text, const char* level, const char* color)
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        std::time_t t = system_clock::to_time_t(now);
        int ms = static_cast<int>(duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        char stamp[16];
        std::snprintf(stamp, sizeof(stamp), "%02d:%02d:%02d.%03d", tm.tm_hour, tm.tm_min, tm.tm_sec, ms);

        if (ms_bColors)
            std::cout << stamp << " " << color << "[" << level << "]" << "\033[0m" << ": " << text << std::endl;
        else
            std::cout << stamp << " [" << level << "]: " << text << std::endl;
    }

public:
    // disabled when stdout is redirected to a log file
    static inline bool ms_bColors = true;

    static void info(const char* format, ...)
    {
        char buffer[512];
        va_list args;
        va_start(args, format);
        std::vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        log(buffer, "info", "\033[34m");
    }

    static void warn(const char* format, ...)
    {
        char buffer[512];
        va_list args;
        va_start(args, format);
        std::vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        log(buffer, "warn", "\033[33m");
    }

    static void error(const char* format, ...)
    {
        char buffer[512];
        va_list args;
        va_start(args, format);
        std::vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        log(buffer, "error", "\033[31m");
    }
};
