/* 
 * File:   commandQueue.h
 * Author: pconroy
 *
 * Created on September 27, 2018, 9:54 AM
 */

#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 */
#include <stdio.h>
#include <uthash/utlist.h>


    
    //
//  As Commands come in from the MQTT Broker, to change parameters in 
//  our Solar Charge Controller, I'll create a structure of type "mqttCommand_t"
//
//  The first eleent is the 'command' itself (e.g. 'BT' to set the Battery Type)
//  The next three depend on whether the command needs an INT, FLOAT or STRING
//  parameter.  Only one of the three is used
//
//      (I'll make the two strings arrays so I can strncpy() and avoid
//      malloc/free for now.  32 bytes is more than enough)
typedef struct  mqttCommand {
    char    command[ 32 ];              // e.g BT, HVD, WTL1
    int     iParam;                     // some commands take Int parameters
    double  fParam;                     // some commands take Floating Point parameters
    char    cParam[ 32 ];               // some commands take other types parameters
} mqttCommand_t;



/*
 * Our Command Queue will Hold "elements" of type "element_t"
 * There are three members in the element_t structure:
 *      1.  a Pointer to the "mqttCommand" structure
 *      2.  a Pointer to the previous element
 *      3.  a Pointer to the next element
 * 
 * From Troy Hanson's documentaion:
 * 
 * You can use any structure with these macros, as long as the structure contains a 
 * next pointer. If you want to make a doubly-linked list, the element also needs to have a prev pointer.
 * 
 *  typedef struct element {
        char *name;
        struct element *prev;   //  needed for a doubly-linked list only 
        struct element *next;   // needed for singly- or doubly-linked lists

     } element;
 * 
 * You can name your structure anything. In the example above it is called element. 
 * Within a particular list, all elements must be of the same type.
 */

typedef struct  element {
    mqttCommand_t   *s;         // the embedded structure
    struct element  *prev;      // needed for a doubly-linked list only 
    struct element  *next;      // needed for singly- or doubly-linked lists
} element_t;



extern  void    *createQueue (const int numElements, const int structureSize);
extern  element_t  *addElement (mqttCommand_t *aStructure);
extern  void    *removeElement( void );
extern  void    *removeElementAndWait( void );
extern  void    destroyQueue( void );



#ifdef __cplusplus
}
#endif

#endif /* COMMANDQUEUE_H */

