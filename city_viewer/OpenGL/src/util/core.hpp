#ifndef CORE_HPP
#define CORE_HPP

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// --------------------------------------------------------------------------------

typedef unsigned char ubyte;
typedef unsigned int  uint;
typedef long long     llong;

// --------------------------------------------------------------------------------

#if defined(_MSC_VER) && defined(_DEBUG)
#   define DEBUG_MODE
#elif (defined(__GNUC__) || defined(__clang__)) && !defined(NDEBUG)
#   define DEBUG_MODE
#endif // defined(_MSC_VER) && defined(_DEBUG)

#if defined(_MSC_VER)
#   include <intrin.h>
#   define HALT __debugbreak
#elif defined(__GNUC__) || defined(__clang__)
#   define HALT __builtin_trap
#else
#   define HALT abort
#endif // defined(_MSC_VER)

#ifdef DEBUG_MODE
#   define ASSERT(expr) \
        do { \
            if (!(expr)) { \
                fprintf( \
                    stderr, \
                    "\x1b[97m%s(%d)\033[0m: function \x1b[97m'%s'\033[0m: assertion \x1b[91m%s\033[0m failed.\n", \
                    __FILE__, __LINE__, __func__, #expr \
                ); \
                HALT(); \
            } \
        } while (0)
#else
#   define ASSERT(expr)
#endif // DEBUG_MODE

// --------------------------------------------------------------------------------

enum Text_Color : ubyte {
    TEXT_COLOR_BRIGHT_RED,
    TEXT_COLOR_BRIGHT_GREEN,
    TEXT_COLOR_BRIGHT_YELLOW,
    TEXT_COLOR_COUNT
};

template<typename ...Args>
void _log_debug(const char *file_path, int line_no, const char *func_name,
                const char *prefix, Text_Color color, const char *fmt, Args ...args) {
    static const char *text_color_table[TEXT_COLOR_COUNT] = {
        "\x1b[91m", // TEXT_COLOR_BRIGHT_RED
        "\x1b[92m", // TEXT_COLOR_BRIGHT_GREEN
        "\x1b[93m", // TEXT_COLOR_BRIGHT_YELLOW
    };

    time_t cur_time;
    time(&cur_time);

    struct tm *local_time = localtime(&cur_time);

    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);

    char fmt_buff[256];
    sprintf(
        fmt_buff,
        "\x1b[97m[%s] %s(%d)\033[0m: function \x1b[97m'%s'\033[0m: %s%s\033[0m: %s\n",
        time_str, file_path, line_no, func_name, text_color_table[color], prefix, fmt
    );

    static char buff[256];
    sprintf(buff, fmt_buff, args...);

    fputs(buff, stderr);
}

namespace global {
extern FILE *log_file;
} // namespace global

template<typename ...Args>
void _log_release(const char *file_path, int line_no, const char *func_name,
                  const char *prefix, const char *fmt, Args ...args) {
    time_t cur_time;
    time(&cur_time);

    struct tm *local_time = localtime(&cur_time);

    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);

    char fmt_buff[256];
    sprintf(
        fmt_buff,
        "[%s] %s(%d): function '%s': %s: %s\n",
        time_str, file_path, line_no, func_name, prefix, fmt
    );

    static char buff[256];
    sprintf(buff, fmt_buff, args...);

    fputs(buff, global::log_file);
}

#ifdef DEBUG_MODE
#   define LOG_TRACE(fmt, ...)   _log_debug(__FILE__, __LINE__, __func__, "TRACE", TEXT_COLOR_BRIGHT_GREEN, fmt, ##__VA_ARGS__)
#   define LOG_WARNING(fmt, ...) _log_debug(__FILE__, __LINE__, __func__, "WARNING", TEXT_COLOR_BRIGHT_YELLOW, fmt, ##__VA_ARGS__)
#   define LOG_ERROR(fmt, ...)   _log_debug(__FILE__, __LINE__, __func__, "ERROR", TEXT_COLOR_BRIGHT_RED, fmt, ##__VA_ARGS__)
#else
#   define LOG_TRACE(fmt, ...)   _log_release(__FILE__, __LINE__, __func__, "TRACE", fmt, ##__VA_ARGS__)
#   define LOG_WARNING(fmt, ...) _log_release(__FILE__, __LINE__, __func__, "WARNING", fmt, ##__VA_ARGS__)
#   define LOG_ERROR(fmt, ...)   _log_release(__FILE__, __LINE__, __func__, "ERROR", fmt, ##__VA_ARGS__)
#endif // DEBUG_MODE

// --------------------------------------------------------------------------------

#define MIN(x, y)        ((x) < (y) ? (x) : (y))
#define MAX(x, y)        ((x) > (y) ? (x) : (y))

#define CLAMP(x, l, h)   ((x) < (l) ? (l) : ((x) > (h) ? (h) : (x)))

#define ARRAY_SIZE(a)    (sizeof((a)) / sizeof(*(a)))

#define BIT(x)           (1 << (x))
#define SET_FLAG(x, f)   ((x) |= (f))
#define UNSET_FLAG(x, f) ((x) &= ~(f))
#define FLIP_FLAG(x, f)  ((x) ^= (f))
#define HAS_FLAG(x, f)   (((x) & (f)) != 0)

// --------------------------------------------------------------------------------

#endif // CORE_HPP