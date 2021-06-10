/*********************************************************************
 * 
 *  file:  CriticalSection.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: 
 *	A CCriticalSection is a recursive mutex optimized for speed and locking
 *  within a single process
 *  On windows, a CCriticalSection is significantly faster than other
 *  synchronization primitives, although it offers less flexibility
 *  On linux, the CCriticalSection is implemented using a recursive
 *  pthread_mutex, so on linux, there should be little performance between
 *  a CCriticalSection and a Mutex
 *
 *  #define DEBUG_CRITICAL_SECTIONS
 *  to get more debugging information at run-time
 * 
 *********************************************************************/

#ifndef __CCRITICAL_SECTION_H__
#define __CCRITICAL_SECTION_H__

/*
#include "pSos/mutex.h"
*/

#ifdef POSIX
#include <pthread.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

namespace DIOMEDE_CRITICAL {

/////////////////////////////////////////////////////////////////////////////
class CCriticalSection {
public:
    CCriticalSection();
    ~CCriticalSection();

    //  Name:       Lock
    //  Action:     Attempt to grab/lock the critical section.  If it's already
    //              locked by by another thread, this will block will block until
    //              it's unlocked.  If already owned by this thread, increments
    //              the lock count
    void    Lock();

    //  Name:       Unlock
    //  Action:     To release the critical section.  Each call to lock should be
    //              matched by a call to unlock from the same thread
    //  Pre:        The critical section has been grabbed by Lock
    void    Unlock();

private:
#ifdef WIN32
    CRITICAL_SECTION	m_criticalSection;
#else
    pthread_mutex_t		m_criticalSection;
#endif

#ifdef DEBUG_CRITICAL_SECTIONS
#ifdef Linux
    pthread_t m_owner;
#endif
#ifdef WIN32
    DWORD m_owner;
#endif
    int m_count;
#endif
};

template <class T> class Lock {
    // Use a Lock<CCriticalSection> to lock/unlock a CriticalSection within a scope to guarantee that
    // it will be unlocked.
    //
public:
    Lock( T &m ) : m_obj( m )  { m_obj.Lock(); }
    ~Lock()                    { m_obj.Unlock(); }
private:
    T &m_obj;

    Lock( Lock& );                    // forbid copy
    Lock& operator=( Lock const& );   // forbid assignment
};


}

#ifndef DEBUG_CRITICAL_SECTIONS
////////// Inlines ////////////////////////////////////////////////////////////

inline DIOMEDE_CRITICAL::CCriticalSection::CCriticalSection() {
#ifdef WIN32
    InitializeCriticalSection( &m_criticalSection );
#else
        pthread_mutexattr_t recursive;
        pthread_mutexattr_init(&recursive);
#ifndef Linux
        pthread_mutexattr_settype(&recursive, PTHREAD_MUTEX_RECURSIVE);
#else
        pthread_mutexattr_settype(&recursive, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
        pthread_mutex_init( &m_criticalSection, &recursive );
        pthread_mutexattr_destroy(&recursive);
#endif
}

inline DIOMEDE_CRITICAL::CCriticalSection::~CCriticalSection() {
#ifdef WIN32
    DeleteCriticalSection( &m_criticalSection );
#else
    pthread_mutex_destroy( &m_criticalSection );
#endif
}

inline void DIOMEDE_CRITICAL::CCriticalSection::Lock() {
#ifdef WIN32
    EnterCriticalSection( &m_criticalSection);
#else
    pthread_mutex_lock( &m_criticalSection );
#endif
}

inline void DIOMEDE_CRITICAL::CCriticalSection::Unlock() {
#ifdef WIN32
    LeaveCriticalSection( &m_criticalSection );
#else
    pthread_mutex_unlock( &m_criticalSection );
#endif
}

#endif // !DEBUG_CRITICAL_SECTIONS



#endif // __CCRITICAL_SECTION_H__

