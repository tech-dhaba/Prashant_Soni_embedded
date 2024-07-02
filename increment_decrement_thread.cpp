#include <windows.h>
#include <stdio.h>

#define NUM_ITERATIONS 1000000

volatile LONG sharedNumber = 0;
CRITICAL_SECTION cs;

DWORD WINAPI IncrementThread(LPVOID param) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        EnterCriticalSection(&cs);
        sharedNumber++;
        LeaveCriticalSection(&cs);
        //printf("Increment value of sharedNumber: %ld\n", sharedNumber);
    }
    return 0;
}

DWORD WINAPI DecrementThread(LPVOID param) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        EnterCriticalSection(&cs);
        sharedNumber--;
        LeaveCriticalSection(&cs);
        //printf("Decrement value of sharedNumber: %ld\n", sharedNumber);
    }
    return 0;
}

int main() {
    HANDLE hThread1, hThread2;

    // Initialize the critical section
    InitializeCriticalSection(&cs);

    // Create threads
    hThread1 = CreateThread(NULL, 0, IncrementThread, NULL, 0, NULL);
    hThread2 = CreateThread(NULL, 0, DecrementThread, NULL, 0, NULL);

    // Wait for threads to finish
    WaitForSingleObject(hThread1, INFINITE);
    WaitForSingleObject(hThread2, INFINITE);

    // Clean up
    DeleteCriticalSection(&cs);
    CloseHandle(hThread1);
    CloseHandle(hThread2);

    // Print the final value of sharedNumber
    printf("Final value of sharedNumber: %ld\n", sharedNumber);

    return 0;
}
