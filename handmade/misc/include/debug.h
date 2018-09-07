#ifndef __debug_h__
#define __debug_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifndef DEBUG
#define debug_Message(M, ...)
#else
//Simple message print for debugging
#define debug_Message(M, ...) fprintf(stderr, "[Debug]\t\t%s:%d:\n[Message]\t" M "\n\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

//If errno is 0, print "None", otherwise print the errno for logging
#define debug_CleanErrno() (errno == 0 ? "None" : strerror(errno))

//Logging at a range of severity levels
#define debug_LogError(M, ...) fprintf(stderr, "[Error]\t\t%s:%d:\n[errno: %s]\t" M "\n\n", __FILE__, __LINE__, debug_CleanErrno(), ##__VA_ARGS__)
#define debug_LogWarning(M, ...) fprintf(stderr, "[Warning]\t%s:%d:\n[errno: %s]\t" M "\n\n", __FILE__, __LINE__, debug_CleanErrno(), ##__VA_ARGS__)
#define debug_LogInfo(M, ...) fprintf(stderr, "[Info]\t\t%s:%d:\n[errno: %s]\t" M "\n\n", __FILE__, __LINE__, debug_CleanErrno(), ##__VA_ARGS__)

//General use error check that prints output and jumps to 'error' label
#define debug_CheckError(A, M, ...) if(!(A)){debug_LogError(M, ##__VA_ARGS__); errno = 0; goto error;}

//Check if memory is allocated and print if not
#define debug_CheckMemory(A) debug_CheckError((A), "[Error]\t\tOut of Memory!")

//Use to account for unknown scenarios (example: Switch default case)
#define debug_CheckAgainst(M, ...) {debug_LogError(M, ##__VA_ARGS__); errno = 0; goto error;}

//Check assertion and print debug message
#define debug_Assert(A, M, ...) if(!(A)){debug_Message(M, ##__VA_ARGS__); errno = 0; goto error;}

#endif