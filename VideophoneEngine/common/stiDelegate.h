////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: stiDelegate
//
//  File Name:  stiDelegate.h
//
//  Abstract:
//	A set of macros to make implementing the delegate design pattern as
//	painless as possible for C++. (A "delegate pattern" is where one class
//	exposes a method as if it does something, when in fact, it passes the
//	call on to another object)
//
// Description:
//	Inside the class you wish to act as a delegate, simply forward the
//	method call onward with a matching DELEGATE or DELEGATE_RET function.
//	This assumes:
//		1) That the delegate has a Lock() and Unlock() function
//			(So the delegatee can be changed.)
//		2) That the delegate method should block if the delegatee is NULL
//
// Example:
//	class Adder {
//		int add(int a,int b) { return a + b; }
//	};
//	class Maths {
//		Adder adder;
//		// a DELEGATE() for 2 parameters and a RETurn value:
//		DELEGATE_RET2(&adder,int,add,int,int);
//	}
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __DELEGATE_H_
#define __DELEGATE_H_

#define INLINE_IT inline

#define DELEGATE_RET_CODE(pobj, ret, fn) while(1) { Lock (); if ((pobj) != NULL) {ret retval = (pobj)->fn(); Unlock (); return retval;} Unlock (); }
#define DELEGATE_CODE(pobj, fn) while(1) { Lock (); if ((pobj) != NULL) {(pobj)->fn(); Unlock (); return;} Unlock (); }
#define DELEGATE_RET(pobj, ret, fn) INLINE_IT ret fn() {DELEGATE_RET_CODE(pobj, ret, fn);}
#define DELEGATE(pobj, fn) INLINE_IT void fn() {DELEGATE_CODE(pobj, fn);}
#define DELEGATE_RET_CONST(pobj, ret, fn) INLINE_IT ret fn() const {DELEGATE_RET_CODE(pobj, ret, fn);}
#define DELEGATE_CONST(pobj, fn) INLINE_IT void fn() const {DELEGATE_CODE(pobj, fn);}

#define DELEGATE_RET_CODE1(pobj, ret, fn, v1) while(1) { Lock (); if ((pobj) != NULL) {ret retval = (pobj)->fn(v1); Unlock (); return retval;} Unlock (); }
#define DELEGATE_CODE1(pobj, fn, v1) while(1) { Lock (); if ((pobj) != NULL) {(pobj)->fn(v1); Unlock (); return;} Unlock (); }
#define DELEGATE_RET1(pobj, ret, fn, a1) INLINE_IT ret fn(a1 v1) {DELEGATE_RET_CODE1(pobj, ret, fn, v1);}
#define DELEGATE1(pobj, fn, a1) INLINE_IT void fn(a1 v1) {DELEGATE_CODE1(pobj, fn, v1);}
#define DELEGATE_RET1_CONST(pobj, ret, fn, a1) INLINE_IT ret fn(a1 v1) const {DELEGATE_RET_CODE1(pobj, ret, fn, v1);}
#define DELEGATE1_CONST(pobj, fn, a1) INLINE_IT void fn(a1 v1) const {DELEGATE_CODE1(pobj, fn, v1);}

#define DELEGATE_RET_CODE2(pobj, ret, fn, v1, v2) while(1) { Lock (); if ((pobj) != NULL) {ret retval = (pobj)->fn(v1, v2); Unlock (); return retval;} Unlock (); }
#define DELEGATE_CODE2(pobj, fn, v1, v2) while(1) { Lock (); if ((pobj) != NULL) {(pobj)->fn(v1, v2); Unlock (); return;} Unlock (); }
#define DELEGATE_RET2(pobj, ret, fn, a1, a2) INLINE_IT ret fn(a1 v1, a2 v2) {DELEGATE_RET_CODE2(pobj, ret, fn, v1, v2);}
#define DELEGATE2(pobj, fn, a1, a2) INLINE_IT void fn(a1 v1, a2 v2) {DELEGATE_CODE2(pobj, fn, v1, v2);}
#define DELEGATE_RET2_CONST(pobj, ret, fn, a1, a2) INLINE_IT ret fn(a1 v1, a2 v2) const {DELEGATE_RET_CODE2(pobj, ret, fn, v1, v2);}
#define DELEGATE2_CONST(pobj, fn, a1, a2) INLINE_IT void fn(a1 v1, a2 v2) const {DELEGATE_CODE2(pobj, fn, v1, v2);}

#define DELEGATE_RET_CODE3(pobj, ret, fn, v1, v2, v3) while(1) { Lock (); if ((pobj) != NULL) {ret retval = (pobj)->fn(v1, v2, v3); Unlock (); return retval;} Unlock (); }
#define DELEGATE_CODE3(pobj, fn, v1, v2, v3) while(1) { Lock (); if ((pobj) != NULL) {(pobj)->fn(v1, v2, v3); Unlock (); return;} Unlock (); }
#define DELEGATE_RET3(pobj, ret, fn, a1, a2, a3) INLINE_IT ret fn(a1 v1, a2 v2, a3 v3) {DELEGATE_RET_CODE3(pobj, ret, fn, v1, v2, v3);}
#define DELEGATE3(pobj, fn, a1, a2, a3) INLINE_IT void fn(a1,  v1, a2 v2, a3 v3) {DELEGATE_CODE3(pobj, fn, v1, v2, v3);}
#define DELEGATE_RET3_CONST(pobj, ret, fn, a1, a2, a3) INLINE_IT ret fn(a1 v1, a2 v2, a3 v3) const {DELEGATE_RET_CODE3(pobj, ret, fn, v1, v2, v3);}
#define DELEGATE3_CONST(pobj, fn, a1, a2, a3) INLINE_IT void fn(a1, v1, a2 v2, a3 v3) const {DELEGATE_CODE3(pobj, fn, v1, v2, v3);}

#endif // __DELEGATE_H_
