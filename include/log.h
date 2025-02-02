#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc.h"

// Macro di configurazione
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

// Variabili globali
static FILE *logFile = NULL;
char difficultyStr[10];


// Macro per il logging della configurazione
#define LOGCONFIG(status) {                                                      \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    switch ((status).difficulty) { \
    case 1: strcpy(difficultyStr, "Easy"); break; \
    case 2: strcpy(difficultyStr, "Hard"); break; \
    default: strcpy(difficultyStr, "Unknown"); \
    }                                                                             \
                                                                                 \
    fprintf(logFile, "%s Player Name: %s, Difficulty: %s, Level: %d\n",          \
            date, (status).name, difficultyStr, (status).level);                 \
    fflush(logFile);                                                             \
}

#define LOGSTUATUS(mode) {                                                     \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
     \
    switch (mode) { \
        case PAUSE: \
            fprintf(logFile, "%s Status: Pause.\n", date); \
            break; \
        case PLAY: \
            fprintf(logFile, "%s Status: Play.\n", date); \
            break; \
        default: \
            fprintf(logFile, "%s Status: Unknown.\n", date); \
    }                                                                             \
    fflush(logFile);                                                             \
}

#define LOGNEWMAP(status) {                                                      \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(logFile, "%s New map created.\n", date);                             \
    fprintf(logFile, "\tTarget positions: ");                                      \
    for (int t = 0; t < MAX_TARGET; t++) {                                       \
        if (status.targets.x[t] == 0 && status.targets.y[t] == 0) break;         \
        fprintf(logFile, "(%d, %d) [val: %d] ",                                  \
                status.targets.x[t], status.targets.y[t], status.targets.value[t]); \
    }                                                                            \
    fprintf(logFile, "\n\tObstacle positions: ");                                  \
    for (int t = 0; t < MAX_OBSTACLES; t++) {                                    \
        if (status.obstacles.x[t] == 0 && status.obstacles.y[t] == 0) break;     \
        fprintf(logFile, "(%d, %d) ",                                            \
                status.obstacles.x[t], status.obstacles.y[t]);                   \
    }                                                                            \
    fprintf(logFile, "\n");                                                      \
    fflush(logFile);                                                             \
}

// Macro per loggare la fine del gioco
#define LOGENDGAME(status, inputStatus) {                                        \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(logFile, "%s Level %d not completed. Game end. Total score: %d\n",   \
            date, status.level, inputStatus.score);                              \
    fflush(logFile);                                                             \
}

// Macro per loggare il completamento di un livello
#define LOGLEVEL(status, inputStatus) {                                          \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(logFile, "%s Level %d completed. Level %d started. Actual score: %d\n", \
            date, status.level, status.level + 1, inputStatus.score);            \
    fflush(logFile);                                                             \
}

#define LOGTARGETHIT(status) { \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(logFile, "%s New target hit. Targets hit:\n", date);            \
    for (int t = 0; t < MAX_TARGET; t++) {                                       \
        if (status.targets.value[t] == 0 && t < numTarget+1) {        \
        fprintf(logFile, "(%d, %d)", status.targets.x[t], status.targets.y[t]); \
        } \
    } \
    fprintf(logFile, "\n");\
    fflush(logFile);  \
}    

#define LOGPROCESSDIED(pid) { \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(logFile, "%s Process %d dead\n",   \
            date, pid);                              \
    fflush(logFile);                                                             \
} 

#if USE_DEBUG
#define LOGPLAYERINFO(status) {    \
    LOGCONFIG(status); \
    fprintf(logFile,"Score: %d\n", status.score);   \
}
#else
#define LOGPLAYERINFO(status) {}
#endif

#if USE_DEBUG
#define LOGDRONEINFO(droneInfo) { \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(logFile, "%s Drone info - ", date); \
    fprintf(logFile, "Position (%d, %d) ", droneInfo.x, droneInfo.y); \
    fprintf(logFile, "Speed (%.2f, %.2f) ", droneInfo.speedX, droneInfo.speedY); \
    fprintf(logFile, "Force (%.2f, %.2f) ", droneInfo.forceX, droneInfo.forceY); \
    fprintf(logFile, "\n"); \
    fflush(logFile); \
}
#else
#define LOGDRONEINFO(droneInfo) {}
#endif
#endif // LOG_H
