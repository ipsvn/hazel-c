#pragma once

#include <stdio.h>

#ifdef HAZEL_LOGGING_ENABLE
#   define HAZEL_LOG_ERROR(...) printf("\x1B[31m[%s:%d] ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\x1B[0m\n");
#   ifdef HAZEL_LOGGING_DEBUG
#       define HAZEL_LOG_DEBUG(...) printf("\x1B[37m[%s:%d] ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\x1B[0m\n");
#       define HAZEL_LOG_DEBUG_PRINT_BYTES(tag, input, size, offset) printf ("\x1B[37m[%s:%d] %s (size:%lld) [", __FILE__, __LINE__, tag, size); for (size_t i = 0; i < size; i++) { printf ("%02x, ", input[i + offset]); } printf("]\x1B[0m\n");
#   endif
#endif

#ifndef HAZEL_LOG_ERROR
#   define HAZEL_LOG_ERROR(...) ;
#endif

#ifndef HAZEL_LOG_DEBUG
#   define HAZEL_LOG_DEBUG(...) ;
#endif
#ifndef HAZEL_LOG_DEBUG_PRINT_BYTES
#   define HAZEL_LOG_DEBUG_PRINT_BYTES(tag, input, size, offset) ;
#endif

#ifndef HAZEL_BUFFER_SIZE
#   define HAZEL_BUFFER_SIZE 1024
#endif
