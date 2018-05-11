#include "abFilei.h"
#include "../abFeedback/abFeedback.h"
#include "../abMath/abBit.h"
#include "../abParallel/abParallel.h"

typedef enum abFilei_Loader_Result {
	abFilei_Loader_Result_Unloaded,
	abFilei_Loader_Result_Success,
	abFilei_Loader_Result_Failure
} abFilei_Loader_Result;

#define abFilei_Loader_GPUUploader_None UINT_MAX

typedef struct abFilei_Loader_Request {
	abFile_AssetHandle assetHandle;
	unsigned int gpuUploaderIndex;
	abFilei_Loader_Result result;
} abFilei_Loader_Request;

#define abFilei_Loader_QueueSize 8u
static uint32_t abFilei_Loader_QueueOccupied; // Bits managed by the main thread (not mutexed) - set when submitted, reset when got result.

static abParallel_Mutex abFilei_Loader_Mutex;
// The latter are protected by abFilei_Loader_Mutex.
static abFilei_Loader_Request abFilei_Loader_Queue[abFilei_Loader_QueueSize];
static abParallel_CondEvent abFilei_Loader_Notification;
static uint32_t abFilei_Loader_QueueRequested; // Bits that notify the loaders about new requests - notify one when setting.
static uint32_t abFilei_Loader_QueueCompleted; // Bits that notify the main thread about new results.
static abBool abFilei_Loader_ShutdownThreads; // Set to true notify all to request a shutdown of all threads.

#define abFilei_Loader_ThreadCount 3u
static abParallel_Thread abFilei_Loader_Threads[abFilei_Loader_ThreadCount];

static void abFilei_Loader_ThreadFunction(void * threadIndexAsPointer) {
	for (;;) {
		abParallel_Mutex_Lock(&abFilei_Loader_Mutex);
		if (abFilei_Loader_ShutdownThreads) {
			abParallel_Mutex_Unlock(&abFilei_Loader_Mutex);
			return;
		}
		if (abFilei_Loader_QueueRequested == 0u) {
			// Neither asked to shut down nor requested loading - wait for some input.
			abParallel_CondEvent_Await(&abFilei_Loader_Notification, &abFilei_Loader_Mutex);
			abParallel_Mutex_Unlock(&abFilei_Loader_Mutex);
			continue;
		}
		unsigned int requestIndex = (unsigned int) abBit_LowestOne32(abFilei_Loader_QueueRequested);
		abFilei_Loader_QueueRequested &= ~(1u << requestIndex);
		abParallel_Mutex_Unlock(&abFilei_Loader_Mutex);

		abFilei_Loader_Request * request = &abFilei_Loader_Queue[requestIndex];
		// Processing must be done here.
		request->result = abFilei_Loader_Result_Success;

		abParallel_Mutex_Lock(&abFilei_Loader_Mutex);
		abFilei_Loader_QueueCompleted |= 1u << requestIndex;
		abParallel_Mutex_Unlock(&abFilei_Loader_Mutex);
	}
}

void abFilei_Loader_Init() {
	abFilei_Loader_QueueOccupied = abFilei_Loader_QueueRequested = abFilei_Loader_QueueCompleted = 0u;
	abFilei_Loader_ShutdownThreads = abFalse;
	abParallel_Mutex_Init(&abFilei_Loader_Mutex);
	abParallel_CondEvent_Init(&abFilei_Loader_Notification);
	char threadName[] = "FileLoader0";
	for (unsigned int threadIndex = 0u; threadIndex < abFilei_Loader_ThreadCount; ++threadIndex) {
		threadName[abArrayLength(threadName) - 2u] = '0' + threadIndex;
		if (!abParallel_Thread_Start(&abFilei_Loader_Threads[threadIndex], threadName,
				abFilei_Loader_ThreadFunction, (void *) (size_t) threadIndex)) {
			abFeedback_Crash("abFilei_Loader_Init", "Failed to start a loader thread.");
		}
	}
}

void abFilei_Loader_Shutdown() {
	abParallel_Mutex_Lock(&abFilei_Loader_Mutex);
	abFilei_Loader_ShutdownThreads = abTrue;
	abParallel_CondEvent_SignalAll(&abFilei_Loader_Notification);
	abParallel_Mutex_Unlock(&abFilei_Loader_Mutex);
	for (unsigned int threadIndex = 0u; threadIndex < abFilei_Loader_ThreadCount; ++threadIndex) {
		abParallel_Thread_Destroy(&abFilei_Loader_Threads[threadIndex]);
	}
}
