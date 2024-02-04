#pragma once

#include <stdio.h>

#define VA_ARGS(...) , ##__VA_ARGS__

#define debug(...)
#define debug2(...)
#define debug3(...)
#define debug4(...)
#define debug5(...)
#define debug_f(...)
#define debug2_f(...)
#define debug3_f(...)
#define debug4_f(...)
#define debug5_f(...)

#define fatal(fmt,...) do { fprintf(stderr, fmt "\n" VA_ARGS(__VA_ARGS__)); abort(); } while(0)
#define error(fmt,...) do { fprintf(stderr, fmt "\n" VA_ARGS(__VA_ARGS__)); } while(0)
#define verbose(fmt,...) do { fprintf(stdout, fmt "\n" VA_ARGS(__VA_ARGS__)); } while(0)
#define fatal_f(fmt,...) do { fprintf(stderr, fmt "\n" VA_ARGS(__VA_ARGS__)); abort(); } while(0)
#define error_f(fmt,...) do { fprintf(stderr, fmt "\n" VA_ARGS(__VA_ARGS__)); } while(0)
#define verbose_f(fmt,...) do {fprintf(stdout, fmt "\n" VA_ARGS(__VA_ARGS__)); } while(0)
