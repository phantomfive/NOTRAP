#ifdef NTP_POSIX_THREADS

struct NTPLock_struct {


};

BOOL NTPStartThread(void (*start_routine)(void *), void *arg);

NTPLock *NTPNewLock();

void NTPFreeLock(NTPLock **lock);

void NTPAcquireLock(NTPLock *lock);

void NTPReleaseLock(NTPLock *lock);

#endif
