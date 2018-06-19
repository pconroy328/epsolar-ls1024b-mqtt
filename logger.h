/* 
 * File:   logger.h
 * Author: pconroy
 *
 * Created on October 25, 2013, 12:21 PM
 */

#ifndef LOGGER_H
#define	LOGGER_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#ifndef  FALSE
# define FALSE 0
# define TRUE  (!FALSE)
#endif
    
    
extern  void    Logger_Initialize( char *fileName, int debugValue );
extern  void    Logger_Terminate();
extern  void    Logger_LogInfo( char *format, ... );
extern  void    Logger_LogDebug( char *format, ... );
extern  void    Logger_LogWarning( char *format, ... );
extern  void    Logger_LogError( char *format, ... );
extern  void    Logger_LogFatal( char *format, ... );


#define Logger_FunctionStart(x)     Logger_LogDebug( "%s[%d] :: %s() - enter\n", __FILE__, __LINE__, __func__)
#define Logger_FunctionEnd(x)       Logger_LogDebug( "%s[%d] :: %s() - exit\n", __FILE__, __LINE__, __func__ )

#ifdef	__cplusplus
}
#endif

#endif	/* LOGGER_H */

