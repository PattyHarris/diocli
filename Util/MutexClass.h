//
// MutexClass.h: header file
//
// Copyright (C) Walter E. Capers.  All rights reserved
//
// This source is free to use as you like.  If you make
// any changes please keep me in the loop.  Email them to
// walt.capers@comcast.net.
//
// PURPOSE:
//
//  To implement mutexes as a C++ object
//
// REVISIONS
// =======================================================
// Date: 10.25.07        
// Name: Walter E. Capers
// Description: File creation
//
// Date:
// Name:
// Description:
//
//

#ifndef __MUTEX_CLASS_H__
#define __MUTEX_CLASS_H__

#ifndef WINDOWS
#if defined(WIN32) || defined(WIN64)
#define WINDOWS
#endif
#endif

#ifndef WINDOWS
#include <pthread.h>
#endif

#include "Stdafx.h"
#include "Thread.h"

class CMutexClass
{
private:
#ifdef WINDOWS
	HANDLE m_mutex;
#else
	pthread_mutex_t m_mutex;
#endif
	ThreadId_t m_owner;
public:
	BOOL m_bCreated;
	void Lock();
	void Unlock();
	CMutexClass(void);
	~CMutexClass(void);
};

#endif // __MUTEX_CLASS_H__

