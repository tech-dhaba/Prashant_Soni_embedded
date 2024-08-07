#include <windows.h>
#include <iostream>

#define NUM_THREADS 5
#define NUM_ITERATIONS 100000

// Shared counter
int sharedCounter = 0;

// Mutex, Critical Section, and Semaphore
HANDLE hMutex;
CRITICAL_SECTION cs;
HANDLE hSemaphore;

// Thread function using Mutex
DWORD WINAPI ThreadFunctionMutex(LPVOID lpParam) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Wait for ownership of the mutex
        WaitForSingleObject(hMutex, INFINITE);
        
        // Safely increment the counter
        sharedCounter++;
        
        // Release ownership of the mutex
        ReleaseMutex(hMutex);
    }
    return 0;
}

// Thread function using Critical Section
DWORD WINAPI ThreadFunctionCS(LPVOID lpParam) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Enter the critical section
        EnterCriticalSection(&cs);
        
        // Safely increment the counter
        sharedCounter++;
        
        // Leave the critical section
        LeaveCriticalSection(&cs);
    }
    return 0;
}

// Thread function using Semaphore
DWORD WINAPI ThreadFunctionSemaphore(LPVOID lpParam) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Wait for the semaphore
        WaitForSingleObject(hSemaphore, INFINITE);
        
        // Safely increment the counter
        sharedCounter++;
        
        // Release the semaphore
        ReleaseSemaphore(hSemaphore, 1, NULL);
    }
    return 0;
}

int main() {
    HANDLE hThreads[NUM_THREADS];
    DWORD dwThreadId[NUM_THREADS];

    // Create a mutex
    hMutex = CreateMutex(NULL, FALSE, NULL);
    if (hMutex == NULL) {
        std::cerr << "CreateMutex error: " << GetLastError() << std::endl;
        return 1;
    }

    // Initialize the critical section
    InitializeCriticalSection(&cs);

    // Create a semaphore
    hSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
    if (hSemaphore == NULL) {
        std::cerr << "CreateSemaphore error: " << GetLastError() << std::endl;
        return 1;
    }

    // Create threads using Mutex
    std::cout << "Creating threads using Mutex..." << std::endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        hThreads[i] = CreateThread(
            NULL,
            0,
            ThreadFunctionMutex,
            NULL,
            0,
            &dwThreadId[i]);

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

    // Display the result
    std::cout << "Final counter value using Mutex: " << sharedCounter << std::endl;

    // Reset counter
    sharedCounter = 0;

    // Create threads using Critical Section
    std::cout << "Creating threads using Critical Section..." << std::endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        hThreads[i] = CreateThread(
            NULL,
            0,
            ThreadFunctionCS,
            NULL,
            0,
            &dwThreadId[i]);

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

    // Display the result
    std::cout << "Final counter value using Critical Section: " << sharedCounter << std::endl;

    // Reset counter
    sharedCounter = 0;

    // Create threads using Semaphore
    std::cout << "Creating threads using Semaphore..." << std::endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        hThreads[i] = CreateThread(
            NULL,
            0,
            ThreadFunctionSemaphore,
            NULL,
            0,
            &dwThreadId[i]);

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

    // Display the result
    std::cout << "Final counter value using Semaphore: " << sharedCounter << std::endl;

    // Cleanup
    CloseHandle(hMutex);
    DeleteCriticalSection(&cs);
    CloseHandle(hSemaphore);

    return 0;
}
