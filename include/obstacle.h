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

#if USE_DEBUG
#define LOGPOSITION(drone) { \
    if (!obstFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(obstFile, "%s Drone info. \n", date); \
    fprintf(obstFile, "\tPre-previous position (%d, %d) \n", (int)(drone.previous_x[1]), (int)round(drone.previous_y[1])); \
    fprintf(obstFile, "\tPrevious position (%d, %d) \n", (int)round(drone.previous_x[0]), (int)round(drone.previous_y[0])); \
    fprintf(obstFile, "\tActual position (%d, %d)\n", (int)round(drone.x), (int)round(drone.y)); \
    fflush(obstFile); \
}

#else 
#define LOGPOSITION(drone) { \
    if (!obstFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(obstFile, "%s Position (%d, %d) ", date, (int)round(drone.x), (int)round(drone.y)); \
    fflush(obstFile); \
}
#endif

#define LOGDRONEINFO(dronebb){ \
    if (!obstFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(obstFile, "%s Position (%d, %d) ", date, dronebb.x, dronebb.y); \
    fprintf(obstFile, "Speed (%.2f, %.2f) ", dronebb.speedX, dronebb.speedY); \
    fprintf(obstFile, "Force (%.2f, %.2f) ", dronebb.forceX, dronebb.forceY); \
    fprintf(obstFile, "\n"); \
    fflush(obstFile); \
}

#if USE_DEBUG
#define LOGFORCES(force_d, force_t, force_o) { \
    if (!obstFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(obstFile, "%s Forces on the drone - ", date); \
    fprintf(obstFile, "Drone force (%.2f, %.2f) ", force_d.x, force_d.y); \
    fprintf(obstFile, "Target force (%.2f, %.2f) ", force_t.x, force_t.y); \
    fprintf(obstFile, "Obstacle force (%.2f, %.2f)\n", force_o.x, force_o.y); \
    fflush(obstFile); \
}
#else
#define LOGFORCES(force_d, force_t, force_o) {}
#endif

#endif // OSTACLE_H