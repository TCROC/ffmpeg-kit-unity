#include <dlfcn.h>
#include "libavutil/avstring.h"
#include "libavutil/error.h"
#include "libavformat/unitybuf_wrapper.h"

static void* g_dl_handle = NULL;
static int g_open_count = 0;

static int unitybuf_open(URLContext *h, const char *uri, int flags) {
    static void* dl_handle = NULL;
    static int (*casted_symbol)(URLContext *, const char *, int) = NULL;

    if (g_dl_handle != dl_handle) {
        casted_symbol = NULL;
        dl_handle = g_dl_handle;
    }

    if (g_dl_handle == NULL) {
        g_dl_handle = dlopen("./ffmpegkit.framework/ffmpegkit", RTLD_LAZY);
        if (g_dl_handle == NULL) {
            return -1;
        }
    }

    g_open_count++;

    if (casted_symbol == NULL) {
        void* symbol = dlsym(g_dl_handle, "unitybuf_open");
        const char* const error_message = dlerror();
        if (error_message != NULL) {
            dlclose(g_dl_handle);
            g_dl_handle = NULL;
            g_open_count = 0;
            return -1;
        }
        casted_symbol = symbol;
    }
    return casted_symbol(h, uri, flags);
}

static int unitybuf_close(URLContext *h) {
    static void* dl_handle = NULL;
    static int (*casted_symbol)(URLContext *) = NULL;

    if (g_dl_handle != dl_handle) {
        casted_symbol = NULL;
        dl_handle = g_dl_handle;
    }

    if (g_dl_handle == NULL) {
        return -1;
    }

    if (casted_symbol == NULL) {
        void* symbol = dlsym(g_dl_handle, "unitybuf_close");
        const char* const error_message = dlerror();
        if (error_message != NULL) {
            dlclose(g_dl_handle);
            g_dl_handle = NULL;
            g_open_count = 0;
            return -1;
        }
        casted_symbol = symbol;
    }
    int ret = casted_symbol(h);

    g_open_count--;

    if (g_open_count <= 0) {
        dlclose(g_dl_handle);
        g_dl_handle = NULL;
        g_open_count = 0;
    }

    return ret;
}

static int unitybuf_write(URLContext *h, const unsigned char *buf, int size) {
    static void* dl_handle = NULL;
    static int (*casted_symbol)(URLContext *, const unsigned char *, int) = NULL;

    if (g_dl_handle != dl_handle) {
        casted_symbol = NULL;
        dl_handle = g_dl_handle;
    }

    if (g_dl_handle == NULL) {
        return -1;
    }

    if (casted_symbol == NULL) {
        void* symbol = dlsym(g_dl_handle, "unitybuf_write");
        const char* const error_message = dlerror();
        if (error_message != NULL) {
            dlclose(g_dl_handle);
            g_dl_handle = NULL;
            g_open_count = 0;
            return -1;
        }
        casted_symbol = symbol;
    }
    return casted_symbol(h, buf, size);
}

static int unitybuf_read(URLContext *h, unsigned char *buf, int size) {
    static void* dl_handle = NULL;
    static int (*casted_symbol)(URLContext *, unsigned char *, int) = NULL;

    if (g_dl_handle != dl_handle) {
        casted_symbol = NULL;
        dl_handle = g_dl_handle;
    }

    if (g_dl_handle == NULL) {
        return -1;
    }

    if (casted_symbol == NULL) {
        void* symbol = dlsym(g_dl_handle, "unitybuf_read");
        const char* const error_message = dlerror();
        if (error_message != NULL) {
            dlclose(g_dl_handle);
            g_dl_handle = NULL;
            g_open_count = 0;
            return -1;
        }
        casted_symbol = symbol;
    }
    return casted_symbol(h, buf, size);
}

const URLProtocol ff_unitybuf_protocol = {
    .name           = "unitybuf",
    .url_open       = unitybuf_open,
    .url_close      = unitybuf_close,
    .url_write      = unitybuf_write,
    .url_read       = unitybuf_read,
    .priv_data_size = sizeof(UnitybufContext),
};