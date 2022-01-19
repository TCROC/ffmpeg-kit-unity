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
} UnitybufStates;

typedef struct {
    UnitybufStates *states;
} UnitybufContext;

extern DLL_EXPORT int unitybuf_open(URLContext *h, const char *uri, int flags);
extern DLL_EXPORT int unitybuf_close(URLContext *h);
extern DLL_EXPORT int unitybuf_write(URLContext *h, const unsigned char *buf, int size);
extern DLL_EXPORT int unitybuf_read(URLContext *h, unsigned char *buf, int size);

extern DLL_EXPORT int unitybuf_write_dll(const char *uri, const unsigned char *buf, int size);
extern DLL_EXPORT int unitybuf_read_dll(const char *uri, unsigned char *buf, int size);
extern DLL_EXPORT int unitybuf_clear_dll(const char *uri);
extern DLL_EXPORT int unitybuf_count_dll(const char *uri);

const URLProtocol ff_unitybuf_protocol = {
    .name           = "unitybuf",
    .url_open       = unitybuf_open,
    .url_close      = unitybuf_close,
    .url_write      = unitybuf_write,
    .url_read       = unitybuf_read,
    .priv_data_size = sizeof(UnitybufContext),
};

#endif