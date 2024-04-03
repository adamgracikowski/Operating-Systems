#pragma once

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)             \
    (__extension__({                               \
        long int __result;                         \
        do                                         \
            __result = (long int)(expression);     \
        while (__result == -1L && errno == EINTR); \
        __result;                                  \
    }))

#endif

#define ELAPSED(start, end) (((end).tv_sec - (start).tv_sec) * 1000000000L + (((end).tv_nsec - (start).tv_nsec)))

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define WAR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__))

#define UNUSED(x) (void)(x)