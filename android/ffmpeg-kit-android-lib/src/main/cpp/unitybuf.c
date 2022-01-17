#include <stdlib.h>
#include <string.h>
#include "libavutil/avstring.h"
#include "libavutil/time.h"
#include "libavformat/unitybuf.h"

static UnitybufStates **g_all_contexts;
static size_t g_all_contexts_count = 0;

static int g_is_lock = 0;
static void lock() {
    while (g_is_lock != 0) {
#ifdef _WIN32
        SwitchToThread();
#else
        av_usleep(1000);
#endif
    }
    g_is_lock = 1;
}
static void unlock() {
    g_is_lock = 0;
#ifdef _WIN32
    SwitchToThread();
#else
    av_usleep(0);
#endif
}

DLL_EXPORT int unitybuf_open(URLContext *h, const char *uri, int flags) {
    lock();

    ((UnitybufContext *)h->priv_data)->states = (UnitybufStates *)av_malloc(sizeof(UnitybufStates));
    UnitybufStates *priv_data = ((UnitybufContext *)h->priv_data)->states;

    char *newUri = (char *)av_malloc(sizeof(char) * (strlen(uri) + 1));
    strcpy(newUri, uri);
    priv_data->uri = newUri;
    av_strstart(uri, "unitybuf:", &uri);
    priv_data->data_size = (size_t)atoi(uri);
    if (priv_data->data_size <= 0) {
        unlock();
        return AVERROR(EINVAL);
    }

    priv_data->count = (size_t)0;
    priv_data->position = (size_t)0;
    priv_data->empty_count = (size_t)0;
    priv_data->read_position = (size_t)0;

    priv_data->datas = NULL;
    priv_data->empty_datas = NULL;

    priv_data->clear_count = 0;

    priv_data->flags = flags;

    if (g_all_contexts_count <= 0) {
        g_all_contexts = (UnitybufStates **)av_malloc(sizeof(UnitybufStates *));
        if (g_all_contexts == NULL) {
            unlock();
            return AVERROR(ENOMEM);
        }
        g_all_contexts[0] = priv_data;
        g_all_contexts_count = 1;
    }
    else {
        UnitybufStates **newContexts = (UnitybufStates **)av_malloc(sizeof(UnitybufStates *) * (g_all_contexts_count + 1));
        if (newContexts == NULL) {
            unlock();
            return AVERROR(ENOMEM);
        }
        for (size_t loop = 0; loop < g_all_contexts_count; loop++) {
            newContexts[loop] = g_all_contexts[loop];
        }
        newContexts[g_all_contexts_count] = priv_data;
        av_freep(&g_all_contexts);
        g_all_contexts = newContexts;
        g_all_contexts_count++;
    }

    unlock();

    return 0;
}

static int unitybuf_clear_inner(UnitybufStates *priv_data) {
    UnitybufStates **new_all_contexts = av_malloc(sizeof(UnitybufStates *) * (g_all_contexts_count - 1));
    size_t pos = 0;
    for (size_t loop = 0; loop < g_all_contexts_count; loop++) {
        if (g_all_contexts[loop] != priv_data) {
            new_all_contexts[pos] = g_all_contexts[loop];
            pos++;
        }
    }
    g_all_contexts = new_all_contexts;
    g_all_contexts_count--;

    for (size_t loop = 0; loop < priv_data->count; loop++) {
        av_freep(&priv_data->datas[loop]);
    }
    av_freep(&priv_data->datas);

    for (size_t loop = 0; loop < priv_data->empty_count; loop++) {
        av_freep(&priv_data->empty_datas[loop]);
    }
    av_freep(&priv_data->empty_datas);
    
    av_freep(&priv_data->uri);

    av_freep(&priv_data);

    return 0;
}

DLL_EXPORT int unitybuf_close(URLContext *h) {
    lock();
    UnitybufStates *priv_data = ((UnitybufContext *)h->priv_data)->states;
    int ret = 0;
    priv_data->clear_count++;
    if (priv_data->clear_count >= 2) {
        ret = unitybuf_clear_inner(priv_data);
    }
    unlock();
    return ret;
}

DLL_EXPORT int unitybuf_clear_dll(const char *uri) {
    lock();

    UnitybufStates *priv_data = NULL;

    for (size_t loop = 0; loop < g_all_contexts_count; loop++) {
        if (strcmp(g_all_contexts[loop]->uri, uri) == 0) {
            priv_data = g_all_contexts[loop];
            break;
        }
    }

    if (priv_data == NULL) {
        unlock();
        return -1;
    }

    priv_data->clear_count++;
    if (priv_data->clear_count >= 2) {
        unitybuf_clear_inner(priv_data);
    }

    unlock();
    return 0;
}

static int unitybuf_write_inner(UnitybufStates *priv_data, const unsigned char *buf, int size) {
    int ret = 0;

    while (size > 0) {
        uint8_t *new_data;
        if (priv_data->position > 0) {
            new_data = priv_data->datas[priv_data->count - 1] + priv_data->position;
        }
        else if (priv_data->empty_count <= 0) {
            new_data = av_malloc(sizeof(uint8_t) * priv_data->data_size);
            if (new_data == NULL) {
                return AVERROR(ENOMEM);
            }
        }
        else {
            new_data = priv_data->empty_datas[priv_data->empty_count - 1];
            priv_data->empty_count--;
        }
        int write_size = size;
        if (write_size > priv_data->data_size - priv_data->position) {
            write_size = priv_data->data_size - priv_data->position;
        }

        memcpy(new_data, buf, write_size);

        if (priv_data->position <= 0) {
            uint8_t **new_datas = av_malloc(sizeof(uint8_t *) * (priv_data->count + 1));
            if (new_datas == NULL) {
                return AVERROR(ENOMEM);
            }
            for (size_t loop = 0; loop < priv_data->count; loop++) {
                new_datas[loop] = priv_data->datas[loop];
            }
            new_datas[priv_data->count] = new_data;
            if (priv_data->datas != NULL) {
                av_freep(&priv_data->datas);
            }
            priv_data->datas = new_datas;
            priv_data->count++;
        }

        if (write_size < priv_data->data_size - priv_data->position) {
            priv_data->position += write_size;
        }
        else {
            priv_data->position = 0;
        }
        size -= write_size;
        buf += write_size;
        ret += write_size;
    }

    if (ret <= 0 && priv_data->clear_count == 0) {
        ret = AVERROR(EAGAIN);
    }

    return ret;
}

DLL_EXPORT int unitybuf_write(URLContext *h, const unsigned char *buf, int size) {
    /*
    if (g_is_lock != 0) {
        return AVERROR(EAGAIN);
    }
    */
    lock();
    int ret = unitybuf_write_inner(((UnitybufContext *)h->priv_data)->states, buf, size);
    unlock();
    return ret;
}

DLL_EXPORT int unitybuf_write_dll(const char *uri, const unsigned char *buf, int size) {
    lock();
    int ret = AVERROR(EINVAL);
    for (size_t loop = 0; loop < g_all_contexts_count; loop++) {
        if (strcmp(g_all_contexts[loop]->uri, uri) == 0) {
            ret = unitybuf_write_inner(g_all_contexts[loop], buf, size);
            break;
        }
    }
    unlock();
    return ret;
}

static int unitybuf_read_inner(UnitybufStates *priv_data, unsigned char *buf, int size) {
    int read_size = 0;
    while (size >= (priv_data->data_size - priv_data->read_position) && priv_data->count > (priv_data->position <= 0 ? 0 : 1)) {
        memcpy(buf, priv_data->datas[0] + priv_data->read_position, priv_data->data_size - priv_data->read_position);
        
        uint8_t **new_empty = av_malloc(sizeof(uint8_t *) * (priv_data->empty_count + 1));
        if (new_empty == NULL) {
            return AVERROR_EOF;
        }
        for (size_t loop = 0; loop < priv_data->empty_count; loop++) {
            new_empty[loop] = priv_data->empty_datas[loop];
        }
        new_empty[priv_data->empty_count] = priv_data->datas[0];
        if (priv_data->empty_datas != NULL) {
            av_freep(&priv_data->empty_datas);
        }
        priv_data->empty_datas = new_empty;
        priv_data->empty_count++;

        uint8_t **new_datas = av_malloc(sizeof(uint8_t *) * (priv_data->count - 1));
        if (new_datas == NULL) {
            return AVERROR_EOF;
        }
        for (size_t loop = 0; loop < priv_data->count - 1; loop++) {
            new_datas[loop] = priv_data->datas[loop + 1];
        }
        av_freep(&priv_data->datas);
        priv_data->datas = new_datas;
        priv_data->count--;

        size -= priv_data->data_size - priv_data->read_position;
        buf += priv_data->data_size - priv_data->read_position;
        read_size += priv_data->data_size - priv_data->read_position;

        priv_data->read_position = (size_t)0;
    }

    if (size > 0 && size < priv_data->data_size - priv_data->read_position && priv_data->count > (priv_data->position <= 0 ? 0 : 1)) {
        memcpy(buf, priv_data->datas[0] + priv_data->read_position, size);
        priv_data->read_position += size;
        read_size += size;
    }

    if (read_size <= 0 && priv_data->clear_count == 0) {
        read_size = AVERROR(EAGAIN);
    }

    return read_size;
}

DLL_EXPORT int unitybuf_read(URLContext *h, unsigned char *buf, int size) {
    /*
    if (g_is_lock != 0) {
        return AVERROR(EAGAIN);
    }
    */
    lock();
    int ret = unitybuf_read_inner(((UnitybufContext *)h->priv_data)->states, buf, size);
    unlock();
    return ret;
}

DLL_EXPORT int unitybuf_read_dll(const char *uri, unsigned char *buf, int size) {
    /*
    if (g_is_lock != 0) {
        return 0;
    }
    */
    lock();
    int ret = 0;
    for (size_t loop = 0; loop < g_all_contexts_count; loop++) {
        if (strcmp(g_all_contexts[loop]->uri, uri) == 0) {
            ret = unitybuf_read_inner(g_all_contexts[loop], buf, size);
            break;
        }
    }
    unlock();
    return ret;
}

DLL_EXPORT int unitybuf_count_dll(const char *uri) {
    lock();
    int ret = 0;
    for (size_t loop = 0; loop < g_all_contexts_count; loop++) {
        if (strcmp(g_all_contexts[loop]->uri, uri) == 0) {
            ret = g_all_contexts[loop]->count - (g_all_contexts[loop]->position <= 0 ? 0 : 1);
            break;
        }
    }
    unlock();
    return ret;
}
