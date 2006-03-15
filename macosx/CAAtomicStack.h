/*=============================================================================
	TStack.h
	
	$Log$
	Revision 1.1.2.1  2006/03/15 20:24:58  baron
	Initial Checkin

	Revision 1.2  2005/11/08 19:50:57  dwyatt
	add pop_all_reversed
	
	Revision 1.1  2005/09/15 23:29:56  dwyatt
	CAAtomicFIFO.h => CAAtomicStack.h
	
	Revision 1.4  2005/03/11 01:41:01  dwyatt
	renaming
	
	Revision 1.3  2004/09/30 21:02:59  jcm10
	add missing includes
	
	Revision 1.2  2004/06/02 22:39:45  dwyatt
	add empty()
	
	Revision 1.1  2004/05/26 00:33:09  dwyatt
	moved from Source/CAServices/AudioUnits/Generators/TStack.h
	
	Revision 1.2  2004/01/07 22:29:00  dwyatt
	work in progress
	
	Revision 1.1  2003/12/12 22:16:25  dwyatt
	initial checkin
	
	created Tue Oct 28 2003, Doug Wyatt
	Copyright (c) 2003 Apple Computer, Inc.  All Rights Reserved

	$NoKeywords: $
=============================================================================*/

#ifndef __TStack_h__
#define __TStack_h__

#if __LP64__
	#include <libkern/OSAtomic.h>
#elif !defined(__COREAUDIO_USE_FLAT_INCLUDES__)
	#include <CoreServices/CoreServices.h>
#else
	#include <DriverSynchronization.h>
#endif

//  linked list FIFO stack, elements are pushed and popped atomically
//  class T must implement set_next() and get_next()
template <class T>
class TAtomicStack {
public:
	TAtomicStack() : mHead(NULL) { }
	
	// non-atomic routines, for use when initializing/deinitializing, operate NON-atomically
	void	push_NA(T *item)
	{
		item->set_next(mHead);
		mHead = item;
	}
	
	T *		pop_NA()
	{
		T *result = mHead;
		if (result)
			mHead = result->get_next();
		return result;
	}
	
	bool	empty() { return mHead == NULL; }
	
	// atomic routines
	void	push_atomic(T *item)
	{
		T *head;
		do {
			head = mHead;
			item->set_next(head);
		} while (!compare_and_swap(head, item, &mHead));
	}
	
	T *		pop_atomic()
	{
		T *result;
		do {
			if ((result = mHead) == NULL)
				break;
		} while (!compare_and_swap(result, result->get_next(), &mHead));
		return result;
	}
	
	T *		pop_all()
	{
		T *result;
		do {
			if ((result = mHead) == NULL)
				break;
		} while (!compare_and_swap(result, NULL, &mHead));
		return result;
	}
	
	T*		pop_all_reversed()
	{
		TAtomicStack<T> reversed;
		T *p = pop_all(), *next;
		while (p != NULL) {
			next = p->get_next();
			reversed.push_NA(p);
			p = next;
		}
		return reversed.mHead;
	}
	
	bool	compare_and_swap(T *oldvalue, T *newvalue, T **pvalue)
	{
#if __LP64__
		return ::OSAtomicCompareAndSwap64Barrier(int64_t(oldvalue), int64_t(newvalue), (int64_t *)pvalue);
#else
		return ::CompareAndSwap(UInt32(oldvalue), UInt32(newvalue), (UInt32 *)pvalue);
#endif
	}
	
protected:
	T *		mHead;
};

#endif // __TStack_h__
