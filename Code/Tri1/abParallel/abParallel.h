#ifndef abInclude_abParallel
#define abInclude_abParallel

#include "../abCommon.h"

#if defined(abPlatform_OS_Windows)
#include <synchapi.h>
#include <winnt.h>

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

#endif
