#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    if (thread_func_args == NULL)
    {
        ERROR_LOG("Invalid thread parameters");
        return NULL;
    }

    DEBUG_LOG("Thread %lu started with wait_to_obtain_ms: %d, wait_to_release_ms: %d",
              (unsigned long)thread_func_args->thread_id,
              thread_func_args->wait_to_obtain_ms,
              thread_func_args->wait_to_release_ms);

    // Sleep for the specified time before trying to obtain the mutex
    DEBUG_LOG("Thread %lu sleeping for %d milliseconds before obtaining mutex",
                (unsigned long)thread_func_args->thread_id,
                thread_func_args->wait_to_obtain_ms);

    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    DEBUG_LOG("Thread %lu attempting to obtain mutex", (unsigned long)thread_func_args->thread_id);
    if (pthread_mutex_lock(thread_func_args->mutex) != 0)
    {
        ERROR_LOG("Thread %lu failed to lock mutex", (unsigned long)thread_func_args->thread_id);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    DEBUG_LOG("Thread %lu obtained mutex", (unsigned long)thread_func_args->thread_id);

    // Sleep for the specified time while holding the mutex
    DEBUG_LOG("Thread %lu holding mutex for %d milliseconds", (unsigned long)thread_func_args->thread_id, thread_func_args->wait_to_release_ms);
    usleep(thread_func_args->wait_to_release_ms * 1000);

    if (pthread_mutex_unlock(thread_func_args->mutex) != 0)
    {
        ERROR_LOG("Thread %lu failed to unlock mutex", (unsigned long)thread_func_args->thread_id);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    DEBUG_LOG("Thread %lu released mutex", (unsigned long)thread_func_args->thread_id);

    thread_func_args->thread_complete_success = true;
    DEBUG_LOG("Thread %lu completed successfully", (unsigned long)thread_func_args->thread_id);

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    if (thread == NULL || mutex == NULL || wait_to_obtain_ms < 0 || wait_to_release_ms < 0)
    {
        ERROR_LOG("Invalid arguments to start_thread_obtaining_mutex");
        return false;
    }

    // Allocate memory for thread_data structure
    struct thread_data* thread_func_args = malloc(sizeof(struct thread_data));
    if (NULL == thread_func_args)
    {
        ERROR_LOG("Failed to allocate memory for thread_data");
        return false;
    }

    // Initialize the thread_data structure
    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;
    thread_func_args->thread_complete_success = false;
    thread_func_args->thread_id = 0; // Initialize thread_id to 0

    // Create the thread
    int result = pthread_create(thread, NULL, threadfunc, (void*)thread_func_args);
    if (0 != result)
    {
        ERROR_LOG("Failed to create thread: %s", strerror(result));
        free(thread_func_args);
        return false;
    }

    // Successfully created the thread, set the thread ID in the thread_data structure
    thread_func_args->thread_id = *thread;
    DEBUG_LOG("Thread created successfully with ID: %lu", (unsigned long)*thread);

    return true;
}

