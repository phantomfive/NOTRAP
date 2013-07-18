#include <notrap/notrap.h>
#ifdef NTP_POSIX_THREADS

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

struct NTPLock_struct {
	pthread_mutex_t *mutex;
};

BOOL NTPStartThread(void *(*start_routine)(void *), void *arg) {
	pthread_t thread;

	if(
		pthread_create(&thread, NULL, start_routine, arg) == 0  &&
		pthread_detach(thread)                            == 0) {
			return TRUE;
	}
	return FALSE;
}

NTPLock *NTPNewLock() {
	pthread_mutexattr_t attr = {0};
	int type = PTHREAD_MUTEX_RECURSIVE;

	NTPLock *rv = malloc(sizeof(NTPLock));
	if(rv!=NULL) {
		
		if(
			pthread_mutexattr_settype(&attr, type) == 0 &&
			pthread_mutex_init(rv->mutex, &attr)   == 0) {
			return rv;  //success
		}

		free(rv);
		rv = NULL;
	}

	return rv;
}

void NTPFreeLock(NTPLock **lock) {
	if(lock==NULL || *lock == NULL) return;
	pthread_mutex_destroy((*lock)->mutex);
	free(*lock);
	*lock = NULL;
}

BOOL NTPAcquireLock(NTPLock *lock) {
	if(pthread_mutex_lock(lock->mutex)!=0) {
		//errors are so rare here, I almost don't
		//want to force users to check them.
		//What will they do exactly? But you can't
		//write super-stable software if your locks
		//are leaking, so...........
		return FALSE;
	}
	return TRUE;
}

void NTPReleaseLock(NTPLock *lock) {
	pthread_mutex_unlock(lock->mutex);
}

#endif
