#ifndef MUTEX_H
#define MUTEX_H

#ifdef VERBOSE_MUTEX
#include <cstdio>
#endif

#include <pthread.h>

class Mutex {
public:
	Mutex() {
		pthread_mutex_init( &m_mutex, NULL );
	}
	void lock() {
#ifdef VERBOSE_MUTEX
		printf("Mutex %p locked\n", this);
#endif
		pthread_mutex_lock( &m_mutex );
	}
	void unlock() {
#ifdef VERBOSE_MUTEX
		printf("Mutex %p unlocked\n", this);
#endif
		pthread_mutex_unlock( &m_mutex );
	}
private:
	pthread_mutex_t m_mutex;
};


#endif
