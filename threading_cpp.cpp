#include <windows.h>
#include <iostream>

#define NUM_THREADS 5

// Thread function
DWORD WINAPI ThreadFunction(LPVOID lpParam) {
    int threadNum = *(int*)lpParam;
    std::cout << "Thread " << threadNum << " is running." << std::endl;
    return 0;
}

int main() {
    HANDLE hThreads[NUM_THREADS];
    DWORD dwThreadId[NUM_THREADS];
    int threadArgs[NUM_THREADS];

    // Create multiple threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        threadArgs[i] = i + 1;
        hThreads[i] = CreateThread(
            NULL,                   // Default security attributes
            0,                      // Default stack size
            ThreadFunction,         // Thread function
            &threadArgs[i],         // Argument to thread function
            0,                      // Default creation flags
            &dwThreadId[i]);        // Receive thread identifier

        if (hThreads[i] == NULL) {
            std::cerr << "Error creating thread " << i + 1 << std::endl;
            return 1;
        }
    }

    // Wait for all threads to finish
    WaitForMultipleObjects(NUM_THREADS, hThreads, TRUE, INFINITE);

    // Close thread handles
    for (int i = 0; i < NUM_THREADS; ++i) {
        CloseHandle(hThreads[i]);
    }

    std::cout << "All threads have finished execution." << std::endl;

    return 0;
}
