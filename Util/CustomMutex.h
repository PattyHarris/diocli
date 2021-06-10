/*********************************************************************
 * 
 *  file:  CustomMutex.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: custom mutex class 
 * 
 *********************************************************************/

#ifndef __CUSTOM_MUTEX_H__
#define __CUSTOM_MUTEX_H__

#include <stdlib.h>
#include "types.h"

#ifdef WIN32
#include <windows.h>
#elif defined(POSIX)
#include <pthread.h>
#endif

class Condition;

//////////////////////////////////////////////////////////////////////

class Mutex {
    //
    // A Mutex implements a cross-platform mutex.
    //
public:
    Mutex();
    ~Mutex();

    //  Name:       Lock
    //  Action:     Attempt to grab/lock a mutex.  If it's already locked by
    //              by another thread, this will block will block until it's
    //              unlocked.
    void    Lock();

    //  Name:       Unlock
    //  Action:     To return from the function when a mutex is free
    //  Pre:        The mutex has been grabbed by Lock
    void    Unlock();

    friend class Condition;

private:
#ifdef WIN32
    HANDLE          m_mutex;
#else
    pthread_mutex_t m_mutex;
#endif
};

//*****************************************************************************

class MutexLock {
    //
    // Use a MutexLock to lock/unlock a Mutex within a scope to guarantee that
    // it will be unlocked.
    //
public:
    MutexLock( Mutex &m ) : m_mutex( m )        { m_mutex.Lock(); }
    ~MutexLock()                                { m_mutex.Unlock(); }
private:
    Mutex &m_mutex;

    MutexLock( MutexLock& );                    // forbid copy
    MutexLock& operator=( MutexLock const& );   // forbid assignment
};

////////// Inlines ////////////////////////////////////////////////////////////

inline Mutex::Mutex() {
#ifdef WIN32
    if ( !(m_mutex = CreateSemaphore( 0, 1, 1, 0 )) )
        abort();
#else
    if ( pthread_mutex_init( &m_mutex, 0 ) )
        abort();
#endif
}

inline Mutex::~Mutex() {
#ifdef WIN32
    CloseHandle( m_mutex );
#else
    pthread_mutex_destroy( &m_mutex );
#endif
}

inline void Mutex::Lock() {
#ifdef WIN32
    WaitForSingleObject( m_mutex, INFINITE );
#else
    pthread_mutex_lock( &m_mutex );
#endif
}

inline void Mutex::Unlock() {
#ifdef WIN32
    ReleaseSemaphore( m_mutex, 1, 0 );
#else
    pthread_mutex_unlock( &m_mutex );
#endif
}

#endif // __CUSTOM_MUTEX_H__

