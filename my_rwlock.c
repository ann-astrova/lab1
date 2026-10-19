#include "my_rwlock.h"

int my_rwlock_init(my_rwlock_t *lock) {
    int code;

    lock->active_readers = 0;
    lock->waiting_readers = 0;
    lock->waiting_writers = 0;
    lock->writer_active = 0;

    code = pthread_mutex_init(&lock->mutex, NULL);
    if (code != 0) return code;

    code = pthread_cond_init(&lock->readers, NULL);
    if (code != 0) {
        pthread_mutex_destroy(&lock->mutex);
        return code;
    }

    code = pthread_cond_init(&lock->writers, NULL);
    if (code != 0) {
        pthread_cond_destroy(&lock->readers);
        pthread_mutex_destroy(&lock->mutex);
        return code;
    }

    return 0;
}

int my_rwlock_rdlock(my_rwlock_t *lock) {
    int code = pthread_mutex_lock(&lock->mutex);
    if (code != 0) return code;

    lock->waiting_readers++;

    while (lock->writer_active || lock->waiting_writers > 0) {
        code = pthread_cond_wait(&lock->readers, &lock->mutex);
        if (code != 0) {
            lock->waiting_readers--;
            pthread_mutex_unlock(&lock->mutex);
            return code;
        }
    }

    lock->waiting_readers--;
    lock->active_readers++;

    return pthread_mutex_unlock(&lock->mutex);
}

int my_rwlock_wrlock(my_rwlock_t *lock) {
    int code = pthread_mutex_lock(&lock->mutex);
    if (code != 0) return code;

    lock->waiting_writers++;

    while (lock->writer_active || lock->active_readers > 0) {
        code = pthread_cond_wait(&lock->writers, &lock->mutex);
        if (code != 0) {
            lock->waiting_writers--;
            pthread_mutex_unlock(&lock->mutex);
            return code;
        }
    }

    lock->waiting_writers--;
    lock->writer_active = 1;

    return pthread_mutex_unlock(&lock->mutex);
}

int my_rwlock_unlock(my_rwlock_t *lock) {
    int code = pthread_mutex_lock(&lock->mutex);
    if (code != 0) return code;

    if (lock->writer_active) {
        lock->writer_active = 0;
    } else if (lock->active_readers > 0) {
        lock->active_readers--;
    } else {
        pthread_mutex_unlock(&lock->mutex);
        return -1;
    }

    if (lock->waiting_writers > 0) {
        if (!lock->writer_active && lock->active_readers == 0) {
            pthread_cond_signal(&lock->writers);
        }
    } else {
        pthread_cond_broadcast(&lock->readers);
    }

    code = pthread_mutex_unlock(&lock->mutex);
    return code;
}

int my_rwlock_destroy(my_rwlock_t *lock) {
    int code;

    code = pthread_cond_destroy(&lock->readers);
    if (code != 0) return code;

    code = pthread_cond_destroy(&lock->writers);
    if (code != 0) return code;

    return pthread_mutex_destroy(&lock->mutex);
}
