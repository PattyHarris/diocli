/*********************************************************************
 * 
 *  file:  DiomedeDLLInterface.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Classes that allow separation of the DLL API
 *          declarations and their implementations
 *          (see How to Create a Heap-Safe and Binary Compatible DLL
 *          in C++, DigCode).
 * 
 *********************************************************************/

#pragma once

/////////////////////////////////////////////////////////////////////////////
// DiomedeDLLInterface
//      Any DLL API interfaced exposed across a DLL boundry will
//      derive from this class.
class DiomedeDLLInterface
{

public:
    // IMPORTANT: No destructor is declared either here or in the drived class.
    void operator delete(void *p)
    {
        if (p) {
            DiomedeDLLInterface* pDLLInterace = static_cast<DiomedeDLLInterface*>(p);
            pDLLInterace->Destroy();
        }
    }

protected:
    virtual void Destroy() = 0;

}; // End DiomedeDLLInterface

/////////////////////////////////////////////////////////////////////////////
// DiomedeDLLInterface
//      Any DLL API implementaton derives from this class.
template <class Interface>
class DiomedeDLLImplementation : public Interface
{

public:
    virtual ~DiomedeDLLImplementation() {}

    virtual void Destroy() { delete this; }
    void operator delete(void* p) { ::operator delete(p); }

}; // End DiomedeDLLImplementation
