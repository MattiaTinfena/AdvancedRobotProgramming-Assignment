#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc.h"

// Macro di configurazione
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

// Variabili globali
extern FILE *obstFile;

#ifdef USE_DEBUG
#define LOGNEWMAP(status) {                                                      \
    if (!obstFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(obstFile, "%s New map created.\n", date);                             \
    fprintf(obstFile, "\tTarget positions: ");                                      \
    for (int t = 0; t < MAX_TARGET; t++) {                                       \
        if (status.targets.x[t] == 0 && status.targets.y[t] == 0) break;         \
        fprintf(obstFile, "(%d, %d) [val: %d] ",                                  \
                status.targets.x[t], status.targets.y[t], status.targets.value[t]); \
    }                                                                            \
    fprintf(obstFile, "\n\tObstacle positions: ");                                  \
    for (int t = 0; t < MAX_OBSTACLES; t++) {                                    \
        if (status.obstacles.x[t] == 0 && status.obstacles.y[t] == 0) break;     \
        fprintf(obstFile, "(%d, %d) ",                                            \
                status.obstacles.x[t], status.obstacles.y[t]);                   \
    }                                                                            \
    fprintf(obstFile, "\n");                                                      \
    fflush(obstFile);                                                             \
}
#else
#define LOGNEWMAP(status) {                                                      \
    if (!obstFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                       \
    fprintf(obstFile, "Obstacle positions: ");                                  \
    for (int t = 0; t < MAX_OBSTACLES; t++) {                                    \
        if (status.obstacles.x[t] == 0 && status.obstacles.y[t] == 0) break;     \
        fprintf(obstFile, "(%d, %d) ",                                            \
                status.obstacles.x[t], status.obstacles.y[t]);                   \
    }                                                                            \
    fprintf(obstFile, "\n");                                                      \
    fflush(obstFile);                                                             \

}
#endif

#define LOGPROCESSDIED() { \
    if (!obstFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                           \
    fprintf(obstFile, "%s Process dead. Obstacle is quitting\n", date);                              \
    fflush(obstFile);                                                             \
}


#define LOGDRONEINFO(dronebb){ \
    if (!obstFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(obstFile, "%s Drone position (%d, %d)\n", date, dronebb.x, dronebb.y); \
    fflush(obstFile); \
}


#endif // OBSTACLE_H