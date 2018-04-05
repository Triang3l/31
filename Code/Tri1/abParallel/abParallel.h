#ifndef abInclude_abParallel
#define abInclude_abParallel
#include "../abCommon.h"

// Note: acquire is used like "check atomic -> acquire -> read dependencies",
// release is used like "write dependencies -> release -> update atomic".

#define abParallel_Thread_MaxNameLength 15u

#if defined(abPlatform_OS_Windows)
/**************************************************
 * Windows and Visual C parallelization primitives
 **************************************************/
#include <intrin.h>
#include <Windows.h>

#if defined(abPlatform_CPU_x86)
#define abAtomic_Acquire _ReadBarrier
#define abAtomic_Release _WriteBarrier
#elif defined(abPlatform_CPU_Arm)
#define abAtomic_Acquire() { __dmb(_ARM_BARRIER_ISH); _ReadBarrier(); }
#define abAtomic_Release() { _WriteBarrier(); __dmb(_ARM_BARRIER_ISH); }
#else
#error No memory barriers defined for the target CPU on Windows.
#endif

abForceInline uint32_t abAtomic_LoadRelaxedU32(uint32_t const * location) {
	return *((uint32_t const volatile *) location);
}
abForceInline uint32_t abAtomic_LoadAcquireU32(uint32_t const * location) {
	uint32_t value = abAtomic_LoadRelaxedU32(location);
	abAtomic_Acquire();
	return value;
}
abForceInline void abAtomic_StoreRelaxedU32(uint32_t * location, uint32_t value) {
	*((uint32_t volatile *) location) = value;
}
abForceInline void abAtomic_StoreReleaseU32(uint32_t * location, uint32_t value) {
	abAtomic_Release();
	abAtomic_StoreRelaxedU32(location, value);
}
#ifdef abPlatform_CPU_64Bit
abForceInline uint64_t abAtomic_LoadRelaxedU64(uint64_t const * location) {
	return *((uint64_t const volatile *) location);
}
abForceInline uint64_t abAtomic_LoadAcquireU64(uint64_t const * location) {
	uint64_t value = abAtomic_LoadRelaxedU64(location);
	abAtomic_Acquire();
	return value;
}
abForceInline void abAtomic_StoreRelaxedU64(uint64_t * location, uint64_t value) {
	*((uint64_t volatile *) location) = value;
}
#else
abForceInline uint64_t abAtomic_LoadRelaxedU64(uint64_t const * location) {
	return (uint64_t) InterlockedCompareExchangeNoFence64((LONGLONG volatile *) location, 0ll, 0ll);
}
abForceInline uint64_t abAtomic_LoadAcquireU64(uint64_t const * location) {
	return (uint64_t) InterlockedCompareExchangeAcquire64((LONGLONG volatile *) location, 0ll, 0ll);
}
abForceInline void abAtomic_StoreRelaxedU64(uint64_t * location, uint64_t value) {
	InterlockedExchangeNoFence64((LONGLONG volatile *) location, (LONGLONG) value);
}
#endif
abForceInline void abAtomic_StoreReleaseU64(uint64_t * location, uint64_t value) {
	abAtomic_Release();
	abAtomic_StoreRelaxedU64(location, value);
}
abForceInline uint32_t abAtomic_SwapU32(uint32_t * location, uint32_t value) {
	return (uint32_t) InterlockedExchange((LONG volatile *) location, (LONG) value);
}
abForceInline uint64_t abAtomic_SwapU64(uint64_t * location, uint64_t value) {
	return (uint64_t) InterlockedExchange64((LONGLONG volatile *) location, (LONGLONG) value);
}
abForceInline uint32_t abAtomic_CompareSwapU32(uint32_t * location, uint32_t oldValue, uint32_t newValue) {
	return (uint32_t) InterlockedCompareExchange((LONG volatile *) location, (LONG) newValue, (LONG) oldValue);
}
abForceInline uint64_t abAtomic_CompareSwapU64(uint64_t * location, uint64_t oldValue, uint64_t newValue) {
	return (uint64_t) InterlockedCompareExchange64((LONGLONG volatile *) location, (LONGLONG) newValue, (LONGLONG) oldValue);
}
abForceInline uint32_t abAtomic_IncrementU32(uint32_t * location) { return (uint32_t) InterlockedIncrement((LONG volatile *) location); }
abForceInline uint64_t abAtomic_IncrementU64(uint64_t * location) { return (uint64_t) InterlockedIncrement64((LONGLONG volatile *) location); }
abForceInline uint32_t abAtomic_DecrementU32(uint32_t * location) { return (uint32_t) InterlockedDecrement((LONG volatile *) location); }
abForceInline uint64_t abAtomic_DecrementU64(uint64_t * location) { return (uint64_t) InterlockedDecrement64((LONGLONG volatile *) location); }
abForceInline uint32_t abAtomic_AddU32(uint32_t * location, uint32_t value) { return (uint32_t) InterlockedAdd((LONG volatile *) location, (LONG) value); }
abForceInline uint64_t abAtomic_AddU64(uint64_t * location, uint64_t value) { return (uint64_t) InterlockedAdd64((LONGLONG volatile *) location, (LONGLONG) value); }
abForceInline uint32_t abAtomic_AndU32(uint32_t * location, uint32_t value) { return (uint32_t) InterlockedAnd((LONG volatile *) location, (LONG) value); }
abForceInline uint64_t abAtomic_AndU64(uint64_t * location, uint64_t value) { return (uint64_t) InterlockedAnd64((LONGLONG volatile *) location, (LONGLONG) value); }
abForceInline uint32_t abAtomic_OrU32(uint32_t * location, uint32_t value) { return (uint32_t) InterlockedOr((LONG volatile *) location, (LONG) value); }
abForceInline uint64_t abAtomic_OrU64(uint64_t * location, uint64_t value) { return (uint64_t) InterlockedOr64((LONGLONG volatile *) location, (LONGLONG) value); }
abForceInline uint32_t abAtomic_XorU32(uint32_t * location, uint32_t value) { return (uint32_t) InterlockedXor((LONG volatile *) location, (LONG) value); }
abForceInline uint64_t abAtomic_XorU64(uint64_t * location, uint64_t value) { return (uint64_t) InterlockedXor64((LONGLONG volatile *) location, (LONGLONG) value); }

#define abParallel_YieldOnCore YieldProcessor
#define abParallel_YieldOnAllCores() Sleep(0)

typedef CRITICAL_SECTION abParallel_Mutex;
abForceInline abBool abParallel_Mutex_Init(abParallel_Mutex * mutex) { InitializeCriticalSection(mutex); return abTrue; }
#define abParallel_Mutex_Lock EnterCriticalSection
#define abParallel_Mutex_Unlock LeaveCriticalSection
#define abParallel_Mutex_Destroy DeleteCriticalSection

typedef SRWLOCK abParallel_RWLock;
abForceInline abBool abParallel_RWLock_Init(abParallel_RWLock * lock) { InitializeSRWLock(lock); return abTrue; }
#define abParallel_RWLock_LockRead AcquireSRWLockShared
#define abParallel_RWLock_UnlockRead ReleaseSRWLockShared
#define abParallel_RWLock_LockWrite AcquireSRWLockExclusive
#define abParallel_RWLock_UnlockWrite ReleaseSRWLockExclusive
#define abParallel_RWLock_Destroy(lock) {}

typedef CONDITION_VARIABLE abParallel_CondEvent;
#define abParallel_CondEvent_Init InitializeConditionVariable
#define abParallel_CondEvent_Destroy(condEvent) {}
#define abParallel_CondEvent_Await(condEvent, mutex) SleepConditionVariableCS(condEvent, mutex, 0xffffffffu)
#define abParallel_CondEvent_Signal WakeConditionVariable
#define abParallel_CondEvent_SignalAll WakeAllConditionVariable

typedef HANDLE abParallel_Thread;
typedef void * (* abParallel_Thread_Entry)(void * data);
abBool abParallel_Thread_Start(abParallel_Thread * thread, char const * name, abParallel_Thread_Entry entry, void * data);
abForceInline unsigned int abParallel_Thread_Destroy(abParallel_Thread * thread) { return (unsigned int) WaitForSingleObject(*thread, INFINITE); }

#else
#error No parallelization API implementation for the target OS.
#endif

// Versions of atomic operations for different data types.
abForceInline int32_t abAtomic_LoadRelaxedS32(int32_t const * location) { return (int32_t) abAtomic_LoadRelaxedU32((uint32_t const *) location); }
abForceInline int64_t abAtomic_LoadRelaxedS64(int64_t const * location) { return (int32_t) abAtomic_LoadRelaxedU64((uint64_t const *) location); }
abForceInline int32_t abAtomic_LoadAcquireS32(int32_t const * location) { return (int32_t) abAtomic_LoadAcquireU32((uint32_t const *) location); }
abForceInline int64_t abAtomic_LoadAcquireS64(int64_t const * location) { return (int32_t) abAtomic_LoadAcquireU64((uint64_t const *) location); }
abForceInline void abAtomic_StoreRelaxedS32(int32_t * location, int32_t value) { abAtomic_StoreRelaxedU32((uint32_t *) location, (uint32_t) value); }
abForceInline void abAtomic_StoreRelaxedS64(int64_t * location, int64_t value) { abAtomic_StoreRelaxedU64((uint64_t *) location, (uint64_t) value); }
abForceInline void abAtomic_StoreReleaseS32(int32_t * location, int32_t value) { abAtomic_StoreReleaseU32((uint32_t *) location, (uint32_t) value); }
abForceInline void abAtomic_StoreReleaseS64(int64_t * location, int64_t value) { abAtomic_StoreReleaseU64((uint64_t *) location, (uint64_t) value); }
abForceInline int32_t abAtomic_SwapS32(int32_t * location, int32_t value) { return (int32_t) abAtomic_SwapU32((uint32_t *) location, (uint32_t) value); }
abForceInline int64_t abAtomic_SwapS64(int64_t * location, int64_t value) { return (int64_t) abAtomic_SwapU64((uint64_t *) location, (uint64_t) value); }
abForceInline int32_t abAtomic_CompareSwapS32(int32_t * location, int32_t oldValue, int32_t newValue) {
	return (int32_t) abAtomic_CompareSwapU32((uint32_t *) location, (uint32_t) oldValue, (uint32_t) newValue);
}
abForceInline int64_t abAtomic_CompareSwapS64(int64_t * location, int64_t oldValue, int64_t newValue) {
	return (int64_t) abAtomic_CompareSwapU64((uint64_t *) location, (uint64_t) oldValue, (uint64_t) newValue);
}
abForceInline int32_t abAtomic_IncrementS32(int32_t * location) { return (int32_t) abAtomic_IncrementU32((uint32_t *) location); }
abForceInline int64_t abAtomic_IncrementS64(int64_t * location) { return (int64_t) abAtomic_IncrementU64((uint64_t *) location); }
abForceInline int32_t abAtomic_DecrementS32(int32_t * location) { return (int32_t) abAtomic_DecrementU32((uint32_t *) location); }
abForceInline int64_t abAtomic_DecrementS64(int64_t * location) { return (int64_t) abAtomic_DecrementU64((uint64_t *) location); }
abForceInline int32_t abAtomic_AddS32(int32_t * location, int32_t value) { return (int32_t) abAtomic_AddU32((uint32_t *) location, (uint32_t) value); }
abForceInline int64_t abAtomic_AddS64(int64_t * location, int64_t value) { return (int64_t) abAtomic_AddU64((uint64_t *) location, (uint64_t) value); }
abForceInline int32_t abAtomic_AndS32(int32_t * location, int32_t value) { return (int32_t) abAtomic_AndU32((uint32_t *) location, (uint32_t) value); }
abForceInline int64_t abAtomic_AndS64(int64_t * location, int64_t value) { return (int64_t) abAtomic_AndU64((uint64_t *) location, (uint64_t) value); }
abForceInline int32_t abAtomic_OrS32(int32_t * location, int32_t value) { return (int32_t) abAtomic_OrU32((uint32_t *) location, (uint32_t) value); }
abForceInline int64_t abAtomic_OrS64(int64_t * location, int64_t value) { return (int64_t) abAtomic_OrU64((uint64_t *) location, (uint64_t) value); }
abForceInline int32_t abAtomic_XorS32(int32_t * location, int32_t value) { return (int32_t) abAtomic_XorU32((uint32_t *) location, (uint32_t) value); }
abForceInline int64_t abAtomic_XorS64(int64_t * location, int64_t value) { return (int64_t) abAtomic_XorU64((uint64_t *) location, (uint64_t) value); }
#ifdef abPlatform_CPU_64Bit
abForceInline void * abAtomic_LoadRelaxedPtr(void * const * location) { return (void *) abAtomic_LoadRelaxedU64((uint64_t const *) location); }
abForceInline void * abAtomic_LoadAcquirePtr(void * const * location) { return (void *) abAtomic_LoadAcquireU64((uint64_t const *) location); }
abForceInline void abAtomic_StoreRelaxedPtr(void * * location, void * value) { abAtomic_StoreRelaxedU64((uint64_t *) location, (uint64_t) value); }
abForceInline void abAtomic_StoreReleasePtr(void * * location, void * value) { abAtomic_StoreReleaseU64((uint64_t *) location, (uint64_t) value); }
abForceInline void * abAtomic_SwapPtr(void * * location, void * value) { return (void *) abAtomic_SwapU64((uint64_t *) location, (uint64_t) value); }
abForceInline void * abAtomic_CompareSwapPtr(void * * location, void * oldValue, void * newValue) {
	return (void *) abAtomic_CompareSwapU64((uint64_t *) location, (uint64_t) oldValue, (uint64_t) newValue);
}
#else
abForceInline void * abAtomic_LoadRelaxedPtr(void * const * location) { return (void *) abAtomic_LoadRelaxedU32((uint32_t const *) location); }
abForceInline void * abAtomic_LoadAcquirePtr(void * const * location) { return (void *) abAtomic_LoadAcquireU32((uint32_t const *) location); }
abForceInline void abAtomic_StoreRelaxedPtr(void * * location, void * value) { abAtomic_StoreRelaxedU32((uint32_t *) location, (uint32_t) value); }
abForceInline void abAtomic_StoreReleasePtr(void * * location, void * value) { abAtomic_StoreReleaseU32((uint32_t *) location, (uint32_t) value); }
abForceInline void * abAtomic_SwapPtr(void * * location, void * value) { return (void *) abAtomic_SwapU32((uint32_t *) location, (uint32_t) value); }
abForceInline void * abAtomic_CompareSwapPtr(void * * location, void * oldValue, void * newValue) {
	return (void *) abAtomic_CompareSwapU32((uint32_t *) location, (uint32_t) oldValue, (uint32_t) newValue);
}
#endif

#endif
