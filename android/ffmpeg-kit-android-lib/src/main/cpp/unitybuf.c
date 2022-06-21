#include <stdlib.h>
#include <string.h>
#include "libavutil/avstring.h"
#include "libavutil/time.h"
#include "unitybuf.h"

static UnitybufStates **g_all_contexts = NULL;
static size_t g_all_contexts_count = 0;

static atomic_flag g_is_lock = ATOMIC_FLAG_INIT;
static void lock() {
    while (atomic_flag_test_and_set(&g_is_lock)) {
        av_usleep(1);
    }
}
static int try_lock() {
    if (atomic_flag_test_and_set(&g_is_lock)) {
        return AVERROR(EAGAIN);
    }
    return 0;
}
static void unlock() {
    atomic_flag_clear(&g_is_lock);
    av_usleep(0);
}

static int local_lock(UnitybufStates *states) {
    while (atomic_flag_test_and_set(&states->is_lock)) {
        av_usleep(1);
    }
    return 0;
}
static void local_unlock(UnitybufStates *states) {
    atomic_flag_clear(&states->is_lock);
    av_usleep(0);
}

static void freep(void** ptr) {
    if (ptr == NULL || *ptr == NULL) {
        return;
    }
    av_freep(ptr);
    *ptr = NULL;
}

static UnitybufStates *unitybuf_get_handle(const char *uri) {
    UnitybufStates *priv_data = NULL;

    for (size_t loop = 0; loop < g_all_contexts_count; loop++) {
        if (g_all_contexts[loop] != NULL && g_all_contexts[loop]->uri != NULL && strcmp(g_all_contexts[loop]->uri, uri) == 0) {
            priv_data = g_all_contexts[loop];
            break;
        }
    }

    return priv_data;
}

DLL_EXPORT UnitybufStates *unitybuf_get_handle_dll(const char *uri) {
    if (uri == NULL) {
        return NULL;
    }

    UnitybufStates *priv_data = NULL;

    //lock();
    if (try_lock() != 0) {
        return NULL;
    }
    priv_data = unitybuf_get_handle(uri);
    unlock();

    return priv_data;
}

DLL_EXPORT int unitybuf_open(URLContext *h, const char *uri, int flags) {
    int loopFlag = 0;
    UnitybufStates *priv_data = NULL;

    do {
        loopFlag = 0;

        ((UnitybufContext *)h->priv_data)->states = (UnitybufStates *)av_mallocz(sizeof(UnitybufStates));
        if (((UnitybufContext *)h->priv_data)->states == NULL) {
            av_usleep(1);
            loopFlag = 1;
            continue;
        }
        priv_data = ((UnitybufContext *)h->priv_data)->states;

        char *newUri = (char *)av_mallocz(sizeof(char) * (strlen(uri) + 1));
        if (newUri == NULL) {
            freep((void**)&priv_data);
            av_usleep(1);
            loopFlag = 1;
            continue;
        }
        strcpy(newUri, uri);
        priv_data->uri = newUri;
        char *uri2 = (char *)av_mallocz(sizeof(char) * (strlen(uri) + 1));
        if (uri2 == NULL) {
            freep((void**)&priv_data->uri);
            freep((void**)&priv_data);
            av_usleep(1);
            loopFlag = 1;
            continue;
        }
        strcpy(uri2, uri + 9);
        char *str_num_ptr = strtok(uri2, "/");
        priv_data->data_size = (size_t)atoi(str_num_ptr);
        str_num_ptr = strtok(NULL, "/");
        priv_data->max_count = (size_t)atoi(str_num_ptr);
        if (priv_data->max_count <= 0) {
            priv_data->max_count = INT_MAX - 1;
        }
        freep((void**)&uri2);

        priv_data->count = (size_t)0;
        priv_data->read_position = (size_t)0;

        priv_data->datas = NULL;
        priv_data->data_lengths = NULL;

        priv_data->clear_count = 0;

        priv_data->flags = flags;

        atomic_flag newFlag = ATOMIC_FLAG_INIT;
        priv_data->is_lock = newFlag;
    } while (loopFlag != 0);

    lock();

    do {
        loopFlag = 0;
        if (g_all_contexts_count <= 0) {
            if (g_all_contexts != NULL) {
                freep((void**)&g_all_contexts);
            }
            g_all_contexts = (UnitybufStates **)av_mallocz(sizeof(UnitybufStates *));
            if (g_all_contexts == NULL) {
                unlock();
                lock();
                loopFlag = 1;
                continue;
            }
            g_all_contexts[0] = priv_data;
            g_all_contexts_count = 1;
        }
        else {
            UnitybufStates **newContexts = (UnitybufStates **)av_mallocz(sizeof(UnitybufStates *) * (g_all_contexts_count + 1));
            if (newContexts == NULL) {
                unlock();
                lock();
                loopFlag = 1;
                continue;
            }
            size_t pos = 0;
            for (size_t loop = 0; loop < g_all_contexts_count; loop++) {
                if (g_all_contexts[loop] != NULL) {
                    newContexts[pos] = g_all_contexts[loop];
                    pos++;
                }
            }
            newContexts[pos] = priv_data;
            freep((void**)&g_all_contexts);
            g_all_contexts = newContexts;
            g_all_contexts_count = pos + 1;
        }
    } while (loopFlag != 0);

    unlock();

    return 0;
}

static int unitybuf_clear_inner(UnitybufStates *priv_data) {
    if (priv_data == NULL) {
        return 0;
    }

    for (size_t loop = 0; loop < g_all_contexts_count; loop++) {
        if (g_all_contexts[loop] == priv_data) {
            g_all_contexts[loop] = NULL;
        }
    }

    if (priv_data->datas != NULL) {
        for (size_t loop = 0; loop < priv_data->count; loop++) {
            freep((void**)&priv_data->datas[loop]);
        }
        freep((void**)&priv_data->datas);
    }

    if (priv_data->data_lengths != NULL) {
        freep((void**)&priv_data->data_lengths);
    }
    
    if (priv_data->uri != NULL) {
        freep((void**)&priv_data->uri);
    }

    freep((void**)&priv_data);

    return 0;
}

DLL_EXPORT int unitybuf_close(URLContext *h) {
    if (h == NULL || h->priv_data == NULL || ((UnitybufContext *)h->priv_data)->states == NULL) {
        return AVERROR(EINVAL);
    }

    UnitybufStates *priv_data = ((UnitybufContext *)h->priv_data)->states;
    
    int ret = 0;
    priv_data->clear_count++;
    if (priv_data->clear_count >= 2) {
        lock();
        ret = unitybuf_clear_inner(priv_data);
        unlock();
    }
    return ret;
}

DLL_EXPORT int unitybuf_clear_dll(UnitybufStates *priv_data) {
    if (priv_data == NULL) {
        return AVERROR(EINVAL);
    }

    priv_data->clear_count++;
    if (priv_data->clear_count >= 2) {
        lock();
        unitybuf_clear_inner(priv_data);
        unlock();
    }

    return 0;
}

static int remove_datas(UnitybufStates *priv_data, int del_count) {
    if (priv_data == NULL) {
        return AVERROR(EINVAL);
    }

    if (priv_data->count - del_count <= 0) {
        if (priv_data->datas != NULL) {
            for (size_t loop = 0; loop < priv_data->count; loop++) {
                freep((void**)&priv_data->datas[loop]);
            }
            freep((void**)&priv_data->datas);
        }
        if (priv_data->data_lengths != NULL) {
            freep((void**)&priv_data->data_lengths);
        }
        priv_data->datas = NULL;
        priv_data->data_lengths = NULL;
        priv_data->count = 0;

        return 0;
    }

    uint8_t **new_datas = av_mallocz(sizeof(uint8_t *) * (priv_data->count - del_count));
    if (new_datas == NULL) {
        return AVERROR(ENOMEM);
    }
    size_t *new_data_lengths = av_mallocz(sizeof(size_t) * (priv_data->count - del_count));
    if (new_data_lengths == NULL) {
        return AVERROR(ENOMEM);
    }
    for (size_t loop = 0; loop < priv_data->count - del_count; loop++) {
        new_datas[loop] = priv_data->datas[loop + del_count];
        new_data_lengths[loop] = priv_data->data_lengths[loop + del_count];
    }
    if (priv_data->datas != NULL) {
        for (size_t loop = 0; loop < del_count; loop++) {
            freep((void**)&priv_data->datas[loop]);
        }
        freep((void**)&priv_data->datas);
    }
    if (priv_data->data_lengths != NULL) {
        freep((void**)&priv_data->data_lengths);
    }
    priv_data->datas = new_datas;
    priv_data->data_lengths = new_data_lengths;
    priv_data->count -= del_count;

    return 0;
}

static int unitybuf_count(UnitybufStates *handle) {
    if (handle == NULL || handle->data_lengths == NULL) {
        return 0;
    }

    if (handle->data_size <= 0) {
        int ret = handle->count;
        return ret;
    }
    else {
        if (handle->count <= 0) {
            return 0;
        }
        int all_size = handle->data_lengths[0] - handle->read_position;
        for (int loop = 1; loop < handle->count; loop++) {
            all_size += handle->data_lengths[loop];
        }
        return all_size / handle->data_size;
    }
    return 0;
}

static int unitybuf_write_inner(UnitybufStates *priv_data, const unsigned char *buf, int size) {
    if (priv_data == NULL) {
        return AVERROR(EINVAL);
    }

    if (size <= 0) {
        return 0;
    }

    int del_count = unitybuf_count(priv_data) - priv_data->max_count;
    if (del_count > 0) {
        return AVERROR(EAGAIN);
    }

    uint8_t *new_data;
    new_data = av_mallocz(sizeof(uint8_t) * size);
    if (new_data == NULL) {
        return AVERROR(ENOMEM);
    }
    int write_size = size;

    memcpy(new_data, buf, write_size);

    uint8_t **new_datas = av_mallocz(sizeof(uint8_t *) * (priv_data->count + 1));
    if (new_datas == NULL) {
        return AVERROR(ENOMEM);
    }
    size_t *new_data_lengths = av_mallocz(sizeof(size_t) * (priv_data->count + 1));
    if (new_data_lengths == NULL) {
        return AVERROR(ENOMEM);
    }
    for (size_t loop = 0; loop < priv_data->count; loop++) {
        new_datas[loop] = priv_data->datas[loop];
        new_data_lengths[loop] = priv_data->data_lengths[loop];
    }
    new_datas[priv_data->count] = new_data;
    new_data_lengths[priv_data->count] = write_size;
    if (priv_data->datas != NULL) {
        freep((void**)&priv_data->datas);
    }
    if (priv_data->data_lengths != NULL) {
        freep((void**)&priv_data->data_lengths);
    }
    priv_data->datas = new_datas;
    priv_data->data_lengths = new_data_lengths;
    priv_data->count++;

    return write_size;
}

DLL_EXPORT int unitybuf_write(URLContext *h, const unsigned char *buf, int size) {
    if (h == NULL || h->priv_data == NULL || ((UnitybufContext *)h->priv_data)->states == NULL) {
        return AVERROR(EINVAL);
    }

    UnitybufStates *priv_data = ((UnitybufContext *)h->priv_data)->states;
    int lock_ret = local_lock(priv_data);
    if (lock_ret < 0) {
        return lock_ret;
    }
    int ret = unitybuf_write_inner(priv_data, buf, size);
    local_unlock(priv_data);
    return ret;
}

DLL_EXPORT int unitybuf_write_dll(UnitybufStates *priv_data, const unsigned char *buf, int size) {
    int ret = AVERROR(EINVAL);

    if (priv_data != NULL) {
        int lock_ret = local_lock(priv_data);
        if (lock_ret < 0) {
            return lock_ret;
        }
        if (priv_data->clear_count > 0 && unitybuf_count(priv_data) <= 0) {
            local_unlock(priv_data);
            return AVERROR_EOF;
        }
        ret = unitybuf_write_inner(priv_data, buf, size);
        local_unlock(priv_data);
    }

    return ret;
}

static int unitybuf_read_inner(UnitybufStates *priv_data, unsigned char *buf, int size) {
    if (priv_data == NULL) {
        return AVERROR(EINVAL);
    }
    if (priv_data->datas == NULL || priv_data->datas[0] == NULL || priv_data->data_lengths == NULL) {
        return AVERROR(EAGAIN);
    }

    if (priv_data->data_size <= 0) {
        if (size <= 0 || priv_data->count <= 0) {
            return AVERROR(EAGAIN);
        }

        int memcpy_size = priv_data->data_lengths[0] - priv_data->read_position;
        if (memcpy_size > size) {
            memcpy_size = size;
        }
        memcpy(buf, &priv_data->datas[0][priv_data->read_position], memcpy_size);

        priv_data->read_position += memcpy_size;
        if (priv_data->read_position >= priv_data->data_lengths[0]) {
            priv_data->read_position -= priv_data->data_lengths[0];

            int remove_result = remove_datas(priv_data, 1);
            if (remove_result < 0) {
                return remove_result;
            }
        }
        
        return memcpy_size;
    }
    else {
        int all_size = -priv_data->read_position;
        int count = 0;
        int end = 0;
        for (int loop = 0; loop < priv_data->count; loop++) {
            all_size += priv_data->data_lengths[loop];
            if (all_size >= priv_data->data_size) {
                count = loop + 1;
                end = all_size - priv_data->data_size;
                break;
            }
        }

        if (count == 0) {
            return AVERROR(EAGAIN);
        }

        int pos = 0;
        for (int loop = 0; loop < count - 1; loop++) {
            memcpy(buf + pos, priv_data->datas[loop] + priv_data->read_position, priv_data->data_lengths[loop] - priv_data->read_position);
            pos += priv_data->data_lengths[loop] - priv_data->read_position;
            priv_data->read_position = 0;
        }
        memcpy(buf + pos, priv_data->datas[count - 1] + priv_data->read_position, priv_data->data_lengths[count - 1] - end - priv_data->read_position);
        pos += priv_data->data_lengths[count - 1] - end - priv_data->read_position;
        priv_data->read_position = 0;

        int remove_result = remove_datas(priv_data, ((end == 0) ? count : (count - 1)));
        if (remove_result < 0) {
            return remove_result;
        }

        if (end > 0) {
            priv_data->read_position = end;
        }

        return pos;
    }
}

DLL_EXPORT int unitybuf_read(URLContext *h, unsigned char *buf, int size) {
    if (h == NULL || h->priv_data == NULL || ((UnitybufContext *)h->priv_data)->states == NULL) {
        return AVERROR(EINVAL);
    }

    UnitybufStates *priv_data = ((UnitybufContext *)h->priv_data)->states;
    int lock_ret = local_lock(priv_data);
    if (lock_ret < 0) {
        return lock_ret;
    }
    int ret;
    //if (priv_data->clear_count > 0 && unitybuf_count(priv_data) <= 0) {
    //    ret = AVERROR_EOF;
    //}
    //else { 
        ret = unitybuf_read_inner(priv_data, buf, size);
    //}
    local_unlock(priv_data);
    if (ret == AVERROR(EAGAIN)) {
        av_usleep(1);
    }
    return ret;
}

DLL_EXPORT int unitybuf_read_dll(UnitybufStates *priv_data, unsigned char *buf, int size) {
    int ret = 0;

    if (priv_data != NULL) {
        int lock_ret = local_lock(priv_data);
        if (lock_ret < 0) {
            return lock_ret;
        }
        if (priv_data->clear_count > 0 && unitybuf_count(priv_data) <= 0) {
            ret = AVERROR_EOF;
        }
        else { 
            ret = unitybuf_read_inner(priv_data, buf, size);
        }
        local_unlock(priv_data);
    }

    return ret;
}

DLL_EXPORT int unitybuf_count_dll(UnitybufStates *handle) {
    if (handle == NULL) {
        return AVERROR(EINVAL);
    }

    int lock_ret = local_lock(handle);
    if (lock_ret < 0) {
        return lock_ret;
    }
    int ret = unitybuf_count(handle);
    local_unlock(handle);
    return ret;
}