#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc.h"

//Macro's configuration
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

//Global variables
extern FILE *wdFile;

/*********************************************************************************************************************/
/********************************************FUNCTIONS TO LOG*********************************************************/
/*********************************************************************************************************************/

#define LOGWDDIED() { \
    if (!wdFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                           \
    fprintf(wdFile, "%s Process dead. The watchdog is quitting\n", date);                              \
    fflush(wdFile);                                                             \
}

#define LOGPROCESSDIED(pid) { \
    if (!wdFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(wdFile, "%s Process %d dead\n",   \
            date, pid);                              \
    fflush(wdFile);                                                             \
} 

#define LOG_PROCESS_NOT_RESPONDING(pid) { \
    if (!wdFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                        \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(wdFile, "%s Process %d not responding. ", date, pid);                  \
    fprintf(wdFile, "Closing everything because process %d is dead. \n", pid);       \
    fflush(wdFile);                                                             \
}


#endif // WATCHDOG_H