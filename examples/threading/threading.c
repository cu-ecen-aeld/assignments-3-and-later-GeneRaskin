#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define DEBUG_LOG(msg, ...) printf("threading: " msg "\n", ##__VA_ARGS__)
#define ERROR_LOG(msg, ...) printf("threading ERROR: " msg "\n", ##__VA_ARGS__)

// Helper function to sleep for milliseconds
void sleep_ms(unsigned int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;                  // Convert to seconds
    ts.tv_nsec = (ms % 1000) * 1000000;     // Convert to nanoseconds
    nanosleep(&ts, NULL);                   // Sleep
}

// Thread function
void* threadfunc(void* thread_param) {
    struct thread_data* thread_func_args = (struct thread_data*)thread_param;

    // Initial sleep before obtaining the mutex
    sleep_ms(thread_func_args->wait_to_obtain_ms);

    // Attempt to lock the mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to lock mutex.");
        thread_func_args->thread_complete_success = false;
        pthread_exit(thread_param); // Exit and return the structure
    }

    // Indicate success
    thread_func_args->thread_complete_success = true;

    // Hold the mutex for the specified time
    sleep_ms(thread_func_args->wait_to_release_ms);

    // Release the mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to unlock mutex.");
        thread_func_args->thread_complete_success = false;
    }

    pthread_exit(thread_param); // Return the thread_data pointer
}

// Start a thread and return immediately without waiting for it to finish
bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms) {
    // Dynamically allocate the thread_data structure
    struct thread_data* thread_func_args = malloc(sizeof(struct thread_data));
    if (!thread_func_args) {
        ERROR_LOG("Failed to allocate memory for thread_data: %s", strerror(errno));
        return false;
    }

    // Initialize thread_data fields
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;
    thread_func_args->mutex = mutex;
    thread_func_args->thread_complete_success = false; // Default to false

    // Create the thread
    if (pthread_create(thread, NULL, threadfunc, thread_func_args) != 0) {
        ERROR_LOG("Failed to create thread: %s", strerror(errno));
        free(thread_func_args); // Free memory on failure
        return false;
    }

    // Return true to indicate the thread was successfully started
    DEBUG_LOG("Thread started successfully.");
    return true;
}
