#ifndef UNITY_BUF_H
#define UNITY_BUF_H

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define DLL_EXPORT __declspec(dllexport)
#include <windows.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define DLL_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define DLL_EXPORT 
#endif

#include "url.h"

typedef struct {
    uint8_t **datas;
    size_t data_size;
    size_t count;
    size_t position;
    uint8_t **empty_datas;
    size_t empty_count;
    const char *uri;
    int flags;
    size_t read_position;
    int clear_count;
    int is_lock;
    size_t max_count;
} UnitybufStates;

typedef struct {
    UnitybufStates *states;
} UnitybufContext;

#endif