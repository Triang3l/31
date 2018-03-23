#ifndef abInclude_abParallel
#define abInclude_abParallel

#include "../abCommon.h"

// Note: acquire is used like "check atomic -> acquire -> read dependencies",
// release is used like "write dependencies -> release -> update atomic".

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

abForceInline uint32_t abAtomic_LoadRelaxedU32(const uint32_t *location) {
	return *((volatile const uint32_t *) location);
}
abForceInline uint32_t abAtomic_LoadAcquireU32(const uint32_t *location) {
	uint32_t value = abAtomic_LoadRelaxedU32(location);
	abAtomic_Acquire();
	return value;
}
abForceInline void abAtomic_StoreRelaxedU32(uint32_t *location, uint32_t value) {
	*((volatile uint32_t *) location) = value;
}
abForceInline void abAtomic_StoreReleaseU32(uint32_t *location, uint32_t value) {
	abAtomic_Release();
	abAtomic_StoreRelaxedU32(location, value);
}

#ifndef abPlatform_CPU_64Bit
// Native 64-bit atomics.
abForceInline uint64_t abAtomic_LoadRelaxedU64(const uint64_t *location) {
	return *((volatile const uint64_t *) location);
}
abForceInline uint64_t abAtomic_LoadAcquireU64(const uint64_t *location) {
	uint64_t value = abAtomic_LoadRelaxedU64(location);
	abAtomic_Acquire();
	return value;
}
abForceInline void abAtomic_StoreRelaxedU64(uint64_t *location, uint64_t value) {
	*((volatile uint64_t *) location) = value;
}
#else
// 64-bit atomics emulated with more relaxed versions of interlocked operations.
abForceInline uint64_t abAtomic_LoadRelaxedU64(const uint64_t *location) {
	return InterlockedCompareExchangeNoFence64((volatile int64_t *) location, 0ll, 0ll);
}
abForceInline uint64_t abAtomic_LoadAcquireU64(const uint64_t *location) {
	return InterlockedCompareExchangeAcquire64((volatile int64_t *) location, 0ll, 0ll);
}
abForceInline void abAtomic_StoreRelaxedU64(uint64_t *location, uint64_t value) {
	InterlockedExchangeNoFence64((volatile int64_t *) location, (int64_t) value);
}
#endif
abForceInline void abAtomic_StoreReleaseU64(uint64_t *location, uint64_t value) {
	abAtomic_Release();
	abAtomic_StoreRelaxedU64(location, value);
}

#define abParallel_YieldOnCore YieldProcessor
#define abParallel_YieldOnAllCores() Sleep(0)
#define abParallel_Pause Sleep

typedef CRITICAL_SECTION abParallel_Mutex;
#define abParallel_Mutex_Init InitializeCriticalSection
#define abParallel_Mutex_Lock EnterCriticalSection
#define abParallel_Mutex_Unlock LeaveCriticalSection
#define abParallel_Mutex_Destroy DeleteCriticalSection

typedef SRWLOCK abParallel_RWLock;
#define abParallel_RWLock_Init InitializeSRWLock
#define abParallel_RWLock_Destroy(lock) {}
#define abParallel_RWLock_LockRead AcquireSRWLockShared
#define abParallel_RWLock_UnlockRead ReleaseSRWLockShared
#define abParallel_RWLock_LockWrite AcquireSRWLockExclusive
#define abParallel_RWLock_UnlockWrite ReleaseSRWLockExclusive

typedef CONDITION_VARIABLE abParallel_CondEvent;
#define abParallel_CondEvent_Init InitializeConditionVariable
#define abParallel_CondEvent_Destroy(condEvent) {}
#define abParallel_CondEvent_Await(condEvent, mutex) SleepConditionVariableCS(condEvent, mutex, 0xffffffffu)
#define abParallel_CondEvent_Signal WakeConditionVariable
#define abParallel_CondEvent_SignalAll WakeAllConditionVariable

#else
#error No parallelization API implementation for the target OS.
#endif

abForceInline int32_t abAtomic_LoadRelaxedS32(const int32_t *location) { return (int32_t) abAtomic_LoadRelaxedU32((const uint32_t *) location); }
abForceInline int64_t abAtomic_LoadRelaxedS64(const int64_t *location) { return (int32_t) abAtomic_LoadRelaxedU64((const uint64_t *) location); }
abForceInline int32_t abAtomic_LoadAcquireS32(const int32_t *location) { return (int32_t) abAtomic_LoadAcquireU32((const uint32_t *) location); }
abForceInline int64_t abAtomic_LoadAcquireS64(const int64_t *location) { return (int32_t) abAtomic_LoadAcquireU64((const uint64_t *) location); }
abForceInline void abAtomic_StoreRelaxedS32(int32_t *location, int32_t value) { abAtomic_StoreRelaxedU32((uint32_t *) location, (uint32_t) value); }
abForceInline void abAtomic_StoreRelaxedS64(int64_t *location, int64_t value) { abAtomic_StoreRelaxedU64((uint64_t *) location, (uint64_t) value); }
abForceInline void abAtomic_StoreReleaseS32(int32_t *location, int32_t value) { abAtomic_StoreReleaseU32((uint32_t *) location, (uint32_t) value); }
abForceInline void abAtomic_StoreReleaseS64(int64_t *location, int64_t value) { abAtomic_StoreReleaseU64((uint64_t *) location, (uint64_t) value); }
#ifdef abPlatform_CPU_64Bit
abForceInline void *abAtomic_LoadRelaxedPtr(void * const *location) { return (void *) abAtomic_LoadRelaxedU64((const uint64_t *) location); }
abForceInline void *abAtomic_LoadAcquirePtr(void * const *location) { return (void *) abAtomic_LoadAcquireU64((const uint64_t *) location); }
abForceInline void abAtomic_StoreRelaxedPtr(void **location, void *value) { abAtomic_StoreRelaxedU64((uint64_t *) location, (uint64_t) value); }
abForceInline void abAtomic_StoreReleasePtr(void **location, void *value) { abAtomic_StoreReleaseU64((uint64_t *) location, (uint64_t) value); }
#else
abForceInline void *abAtomic_LoadRelaxedPtr(void * const *location) { return (void *) abAtomic_LoadRelaxedU32((const uint32_t *) location); }
abForceInline void *abAtomic_LoadAcquirePtr(void * const *location) { return (void *) abAtomic_LoadAcquireU32((const uint32_t *) location); }
abForceInline void abAtomic_StoreRelaxedPtr(void **location, void *value) { abAtomic_StoreRelaxedU32((uint32_t *) location, (uint32_t) value); }
abForceInline void abAtomic_StoreReleasePtr(void **location, void *value) { abAtomic_StoreReleaseU32((uint32_t *) location, (uint32_t) value); }
#endif

#endif
