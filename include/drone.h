#ifndef DRONE_H
#define DRONE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc.h"

// Macro di configurazione
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

// Variabili globali
extern FILE *droneFile;

typedef struct
{
    float x;
    float y;
    float previous_x[2];
    float previous_y[2];

} Drone;

//Functions definition
void updatePosition(Drone *p, Force force, int mass, Speed *speed, Speed *speedPrev);
void drone_force(char* direction);
void obstacle_force(Drone* drone);
void target_force(Drone *drone, Targets* targets);
void boundary_force(Drone *drone);
Force total_force(Force drone, Force obstacle, Force target, Force boundary);
Force compute_repulsive_force(Drone *drone, float x, float y);
void sig_handler(int signo);
void newDrone (Drone* drone, Targets* targets, char* directions, char inst);
void droneUpdate(Drone* drone, Speed* speed, Force* force, Message* msg);
void mapInit(Drone* drone, Message* status);
void readConfig();

/*********************************************************************************************************************/
/********************************************FUNCTIONS TO LOG*********************************************************/
/*********************************************************************************************************************/

#define LOGNEWMAP(status) {                                                      \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(droneFile, "%s New map created.\n", date);                             \
    fprintf(droneFile, "\tTarget positions: ");                                      \
    for (int t = 0; t < MAX_TARGET; t++) {                                       \
        if (status.targets.x[t] == 0 && status.targets.y[t] == 0) break;         \
        fprintf(droneFile, "(%d, %d) [val: %d] ",                                  \
                status.targets.x[t], status.targets.y[t], status.targets.value[t]); \
    }                                                                            \
    fprintf(droneFile, "\n\tObstacle positions: ");                                  \
    for (int t = 0; t < MAX_OBSTACLES; t++) {                                    \
        if (status.obstacles.x[t] == 0 && status.obstacles.y[t] == 0) break;     \
        fprintf(droneFile, "(%d, %d) ",                                            \
                status.obstacles.x[t], status.obstacles.y[t]);                   \
    }                                                                            \
    fprintf(droneFile, "\n");                                                      \
    fflush(droneFile);                                                             \
}


#define LOGPROCESSDIED() { \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                           \
    fprintf(droneFile, "%s Process dead. Drone is quitting\n", date);                              \
    fflush(droneFile);                                                             \
}

#if USE_DEBUG
#define LOGPOSITION(drone) { \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(droneFile, "%s Drone info. \n", date); \
    fprintf(droneFile, "\tPre-previous position (%d, %d) \n", (int)(drone.previous_x[1]), (int)round(drone.previous_y[1])); \
    fprintf(droneFile, "\tPrevious position (%d, %d) \n", (int)round(drone.previous_x[0]), (int)round(drone.previous_y[0])); \
    fprintf(droneFile, "\tActual position (%d, %d)\n", (int)round(drone.x), (int)round(drone.y)); \
    fflush(droneFile); \
}

#else 
#define LOGPOSITION(drone) { \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(droneFile, "%s Position (%d, %d) ", date, (int)round(drone.x), (int)round(drone.y)); \
    fflush(droneFile); \
}
#endif

#define LOGDRONEINFO(dronebb){ \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(droneFile, "%s Position (%d, %d) ", date, dronebb.x, dronebb.y); \
    fprintf(droneFile, "Speed (%.2f, %.2f) ", dronebb.speedX, dronebb.speedY); \
    fprintf(droneFile, "Force (%.2f, %.2f) ", dronebb.forceX, dronebb.forceY); \
    fprintf(droneFile, "\n"); \
    fflush(droneFile); \
}

#if USE_DEBUG
#define LOGFORCES(force_d, force_t, force_o) { \
    if (!droneFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(droneFile, "%s Forces on the drone - ", date); \
    fprintf(droneFile, "Drone force (%.2f, %.2f) ", force_d.x, force_d.y); \
    fprintf(droneFile, "Target force (%.2f, %.2f) ", force_t.x, force_t.y); \
    fprintf(droneFile, "Obstacle force (%.2f, %.2f)\n", force_o.x, force_o.y); \
    fflush(droneFile); \
}
#else
#define LOGFORCES(force_d, force_t, force_o) {}
#endif

#endif // DRONE_H