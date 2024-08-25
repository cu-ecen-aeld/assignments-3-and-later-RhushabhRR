#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter

    bool success = false;
    int err = 0;

    struct thread_data* pThreadData = (struct thread_data *) thread_param;
    if(!pThreadData) ERROR_LOG("Invalid NULL threadfunc parameter");
    else success = true;
    
    
    if(success)
    {
        usleep(pThreadData->wait_to_obtain_ms * 1000);
        err = pthread_mutex_lock(pThreadData->mutex);
        if(err) 
        {
            ERROR_LOG("Failed to lock mutex");
            success = false;
        }
        else
        {
            DEBUG_LOG("Mutex acquired successfully!!!");
            usleep(pThreadData->wait_to_release_ms * 1000);
            err = pthread_mutex_unlock(pThreadData->mutex);
            if(err) 
            {
                ERROR_LOG("Failed to unlock mutex");
                success = false;
            }
            else DEBUG_LOG("Mutex released successfully!!!");
        }
    }
  
    pThreadData->thread_complete_success = success;
    return pThreadData;
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

    bool success = false;
    int err = 0;

    struct thread_data *pThreadData = malloc (sizeof(struct thread_data));
    if(!pThreadData) ERROR_LOG("Failed to dynamically create thread_data");
    else
    {
            pThreadData->mutex = mutex;
            pThreadData->wait_to_obtain_ms = wait_to_obtain_ms;
            pThreadData->wait_to_release_ms = wait_to_release_ms;
            success = true;    
    }
    
    if(success)
    {
        err = pthread_create(thread, NULL, threadfunc, (void *)pThreadData);
        if(err) 
        {
            ERROR_LOG("Failed to create thead");
            success = false;
        }
        else DEBUG_LOG("Thread has been created successfully!!!");
    }

    return success;
}

