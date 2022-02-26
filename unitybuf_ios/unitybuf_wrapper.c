#include <dlfcn.h>
#include "libavutil/avstring.h"
#include "libavutil/error.h"
#include "libavutil/time.h"
#include "libavformat/unitybuf_wrapper.h"

static void* g_dl_handle = NULL;
static int g_open_count = 0;

static atomic_flag g_is_lock = ATOMIC_FLAG_INIT;
static void lock() {
    while (atomic_flag_test_and_set(&g_is_lock)) {
        av_usleep(1);
    }
}
static void unlock() {
    atomic_flag_clear(&g_is_lock);
    av_usleep(0);
}

static int (*g_casted_open_symbol)(URLContext *, const char *, int) = NULL;
static int (*g_casted_close_symbol)(URLContext *) = NULL;
static int (*g_casted_write_symbol)(URLContext *, const unsigned char *, int) = NULL;
static int (*g_casted_read_symbol)(URLContext *, unsigned char *, int) = NULL;

static void reset_symbols() {
    g_casted_open_symbol = NULL;
    g_casted_close_symbol = NULL;
    g_casted_write_symbol = NULL;
    g_casted_read_symbol = NULL;
}

static int unitybuf_open(URLContext *h, const char *uri, int flags) {
    lock();

    if (g_dl_handle == NULL) {
        reset_symbols();
        g_dl_handle = dlopen("ffmpegkit.framework/ffmpegkit", RTLD_LAZY | RTLD_NODELETE | RTLD_FIRST);
        if (g_dl_handle == NULL) {
            unlock();
            return -1;
        }
    }

    g_open_count++;

    if (g_casted_open_symbol == NULL) {
        void* symbol = dlsym(g_dl_handle, "unitybuf_open");
        const char* const error_message = dlerror();
        if (error_message != NULL) {
            dlclose(g_dl_handle);
            g_dl_handle = NULL;
            reset_symbols();
            g_open_count = 0;
            unlock();
            return -1;
        }
        g_casted_open_symbol = symbol;
    }

    int ret = g_casted_open_symbol(h, uri, flags);
    unlock();
    return ret;
}

static int unitybuf_close(URLContext *h) {
    lock();

    if (g_dl_handle == NULL) {
        reset_symbols();
        g_dl_handle = dlopen("ffmpegkit.framework/ffmpegkit", RTLD_LAZY | RTLD_NODELETE | RTLD_FIRST);
        if (g_dl_handle == NULL) {
            unlock();
            return -1;
        }
    }

    if (g_casted_close_symbol == NULL) {
        void* symbol = dlsym(g_dl_handle, "unitybuf_close");
        const char* const error_message = dlerror();
        if (error_message != NULL) {
            dlclose(g_dl_handle);
            g_dl_handle = NULL;
            reset_symbols();
            g_open_count = 0;
            unlock();
            return -1;
        }
        g_casted_close_symbol = symbol;
    }
    int ret = g_casted_close_symbol(h);

    g_open_count--;

    if (g_open_count <= 0) {
        dlclose(g_dl_handle);
        g_dl_handle = NULL;
        reset_symbols();
        g_open_count = 0;
    }

    unlock();
    return ret;
}

static int unitybuf_write(URLContext *h, const unsigned char *buf, int size) {
    lock();

    if (g_dl_handle == NULL) {
        reset_symbols();
        g_dl_handle = dlopen("ffmpegkit.framework/ffmpegkit", RTLD_LAZY | RTLD_NODELETE | RTLD_FIRST);
        if (g_dl_handle == NULL) {
            unlock();
            return -1;
        }
    }

    if (g_casted_write_symbol == NULL) {
        void* symbol = dlsym(g_dl_handle, "unitybuf_write");
        const char* const error_message = dlerror();
        if (error_message != NULL) {
            dlclose(g_dl_handle);
            g_dl_handle = NULL;
            reset_symbols();
            g_open_count = 0;
            unlock();
            return -1;
        }
        g_casted_write_symbol = symbol;
    }

    int ret = g_casted_write_symbol(h, buf, size);
    unlock();
    return ret;
}

static int unitybuf_read(URLContext *h, unsigned char *buf, int size) {
    lock();

    if (g_dl_handle == NULL) {
        reset_symbols();
        g_dl_handle = dlopen("ffmpegkit.framework/ffmpegkit", RTLD_LAZY | RTLD_NODELETE | RTLD_FIRST);
        if (g_dl_handle == NULL) {
            unlock();
            return -1;
        }
    }

    if (g_casted_read_symbol == NULL) {
        void* symbol = dlsym(g_dl_handle, "unitybuf_read");
        const char* const error_message = dlerror();
        if (error_message != NULL) {
            dlclose(g_dl_handle);
            g_dl_handle = NULL;
            reset_symbols();
            g_open_count = 0;
            unlock();
            return -1;
        }
        g_casted_read_symbol = symbol;
    }

    int ret = g_casted_read_symbol(h, buf, size);
    unlock();
    return ret;
}

const URLProtocol ff_unitybuf_protocol = {
    .name           = "unitybuf",
    .url_open       = unitybuf_open,
    .url_close      = unitybuf_close,
    .url_write      = unitybuf_write,
    .url_read       = unitybuf_read,
    .priv_data_size = sizeof(UnitybufContext),
};