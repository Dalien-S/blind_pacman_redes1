#ifndef LOGGING
#define LOGGING
#include <stdio.h>

#define ERROR -1
#define NOERROR 0

namespace color {
extern const char* normal;
extern const char* black;
extern const char* red;
extern const char* green;
extern const char* yellow;
extern const char* blue;
extern const char* magenta;
extern const char* cyan;
extern const char* white;
extern const char* back_black;
extern const char* back_red;
extern const char* back_green;
extern const char* back_yellow;
extern const char* back_blue;
extern const char* back_magenta;
extern const char* back_cyan;
extern const char* back_white;
}  // namespace color

struct Logger {
    FILE* output_file = stderr;

    // Logger();
    // Logger(const char* file_path);
    // ~Logger();
    static Logger initLogger(const char* file_path);
    static Logger initLogger(FILE* file);
    static void terminateLogger(Logger* logger);

    void print(const char* fmt, ...);
    void printColor(const char* color, const char* fmt, ...);
};

#endif
