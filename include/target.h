#ifndef TARGET_H
#define TARGET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc.h"

//Macro's configuration
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

//Global variables
extern FILE *targFile;

//Functions definition
void readConfig();
int canSpawnPrev(int x_pos, int y_pos);
void createTargets();
void refreshMap();
void sig_handler(int signo);

/*********************************************************************************************************************/
/********************************************FUNCTIONS TO LOG*********************************************************/
/*********************************************************************************************************************/

#define LOGNEWMAP(status) {                                                      \
    if (!targFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(targFile, "%s New map created.\n", date);                             \
    fprintf(targFile, "\tTarget positions: ");                                      \
    for (int t = 0; t < MAX_TARGET; t++) {                                       \
        if (status.targets.x[t] == 0 && status.targets.y[t] == 0) break;         \
        fprintf(targFile, "(%d, %d) [val: %d] ",                                  \
                status.targets.x[t], status.targets.y[t], status.targets.value[t]); \
    }                                                                            \
    fprintf(targFile, "\n");                                                      \
    fflush(targFile);                                                             \
}


#define LOGPROCESSDIED() { \
    if (!targFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                           \
    fprintf(targFile, "%s Process dead. Obstacle is quitting\n", date);                              \
    fflush(targFile);                                                             \
}


#define LOGDRONEINFO(dronebb){ \
    if (!targFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(targFile, "%s Drone position (%d, %d)\n", date, dronebb.x, dronebb.y); \
    fflush(targFile); \
}


#endif // TARGET_H