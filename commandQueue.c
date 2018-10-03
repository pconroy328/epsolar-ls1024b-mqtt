/*
 * File:    commandQueue.c
 * author:  patrick conroy
 * 
 * Leverage the wonderful work of Troy Hanson and his Linked List Macros
 * to create a FIFO queue.
 * 
 * NB: No logging in here. One fprintf() to stderr to see if I made a mistake
 * 
 * date:    September 21, 2018
 */
#include <stdio.h>
#include <stdlib.h>
#include <uthash/utlist.h>              // Troy D. Hanson's work!
#include <pthread.h>

#include "commandQueue.h"


static  element_t       *head = NULL;       // head of dbl linked list  MUST BE INIIIALIZED TO NULL

static  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static  pthread_cond_t  condition = PTHREAD_COND_INITIALIZER;


// -----------------------------------------------------------------------------
void    *createQueue (const int numElements, const int structureSize)
{
    //
    //  Ok to call destroy() on one that hasn't been created. Will return EINVAL
    pthread_mutex_destroy( &lock );
    
    pthread_mutex_init( &lock, NULL );
    pthread_mutex_lock( &lock );
    
    if (head != NULL)
        destroyQueue();
    head = NULL;
    
    pthread_mutex_unlock( &lock );
    return head;
}

// -----------------------------------------------------------------------------
element_t *addElement (mqttCommand_t *aStructure)
{
    element_t   *newElement = malloc( sizeof( element_t ) );
    if (newElement != NULL) {
        newElement->s = aStructure;
        
        pthread_mutex_lock( &lock );   
        DL_APPEND( head, newElement );
        pthread_cond_signal( &condition );
        pthread_mutex_unlock( &lock );

        return newElement;
    } 
    
    return NULL;
}

// -----------------------------------------------------------------------------
void    *removeElement ()
{
    if (head != NULL) {
        mqttCommand_t   *aStructure = head->s;
        element_t       *headElement = head;
        
        pthread_mutex_lock( &lock );
        DL_DELETE( head, headElement );
        free( headElement );
        pthread_mutex_unlock( &lock );
    
        return aStructure;
    }
    
    return NULL;
}

// -----------------------------------------------------------------------------
void    destroyQueue ()
{
    element_t   *e1, *e2;

    pthread_mutex_lock( &lock );    
    DL_FOREACH_SAFE( head, e1, e2) {
        if (e1->s != NULL)
            free( e1->s );
        
        DL_DELETE( head, e1 );
        free( e1 );    
    }
    
    free( head );
    head = NULL;
    pthread_mutex_unlock( &lock );
    pthread_mutex_destroy( &lock );
}

// -----------------------------------------------------------------------------
void    *removeElementAndWait ()
{
    mqttCommand_t   *aStructure = NULL;
    
    pthread_mutex_lock( &lock );
    while (head == NULL)
        pthread_cond_wait( &condition, &lock );

    //
    //  Head should not be null now...
    if (head != NULL) {
        aStructure = head->s;
        element_t   *headElement = head;
        
        DL_DELETE( head, headElement );
        free( headElement );
    } else
        fprintf( stderr, "commandQueue : PROGRAMMER FAUX PAUS - removeElementAndWait - head ptr was NULL!\n" );
    
    pthread_mutex_unlock( &lock );
    return aStructure;
}
