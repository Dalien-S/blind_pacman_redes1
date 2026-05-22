#include "logging.hpp"

#include <stdarg.h>

namespace color {
const char* normal = "\x1B[0m";
const char* black = "\x1B[30m";
const char* red = "\x1B[31m";
const char* green = "\x1B[32m";
const char* yellow = "\x1B[33m";
const char* blue = "\x1B[34m";
const char* magenta = "\x1B[35m";
const char* cyan = "\x1B[36m";
const char* white = "\x1B[37m";
const char* back_black = "\x1B[40m";
const char* back_red = "\x1B[41m";
const char* back_green = "\x1B[42m";
const char* back_yellow = "\x1B[43m";
const char* back_blue = "\x1B[44m";
const char* back_magenta = "\x1B[45m";
const char* back_cyan = "\x1B[46m";
const char* back_white = "\x1B[47m";
}  // namespace color

// Logger::Logger() {};
// Logger::Logger(const char* file_path) {
//     this->output_file = fopen(file_path, "w");
//     if (!this->output_file) {
//         fprintf(stderr, "error when opening log file (%s)\n", file_path);
//         this->output_file = stderr;
//     }
// };
//
// Logger::~Logger() {
//     if (this->output_file != stderr) fclose(this->output_file);
// }
Logger Logger::initLogger(const char* file_path) {
    Logger logger = (Logger){.output_file = stderr};

    if (file_path) {
        logger.output_file = fopen(file_path, "w");
        if (!logger.output_file) {
            fprintf(stderr, "error when opening log file (%s)\n", file_path);
            logger.output_file = stderr;
        }
    }

    return logger;
}

Logger Logger::initLogger(FILE* file) {
    Logger logger = (Logger){.output_file = file};
    return logger;
}

void Logger::terminateLogger(Logger* logger){
    if (!logger) return;
    if (!logger->output_file) return;
    if (logger->output_file == stderr) return;
    fclose(logger->output_file);
    logger->output_file = NULL;
}

void Logger::print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(this->output_file, fmt, args);
    va_end(args);
}

void Logger::printColor(const char* color, const char* fmt, ...) {
    va_list args;

    fprintf(this->output_file, "%s", color);
    va_start(args, fmt);
    vfprintf(this->output_file, fmt, args);
    va_end(args);
    fprintf(this->output_file, "%s", color::normal);
}
