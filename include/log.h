#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc.h"

//Macro's configuration
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

//Global variables
extern FILE *logFile;
char difficultyStr[10];

//Functions definition
void sig_handler(int signo);
void storePreviousPosition(Drone_bb *drone);
void resizeHandler();
void resetTargetValue(Message* status);
void mapInit();
void drawDrone(WINDOW * win);
void drawObstacle(WINDOW * win);
void drawTarget(WINDOW * win);
void drawMenu(WINDOW* win);
int randomSelect(int n);
void detectCollision(Message* status, Drone_bb * prev);
void createNewMap();
void closeAll();
void quit();
void readConfig();

/*********************************************************************************************************************/
/********************************************FUNCTIONS TO LOG*********************************************************/
/*********************************************************************************************************************/

#define LOGCONFIG(status) {                                                      \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
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
    fprintf(logFile, "%s Player Name: %s, Difficulty: %s, Level: %d, Score: %d\n",          \
            date, (status).name, difficultyStr, (status).level, status.score);                 \
    fflush(logFile);                                                             \
}

#define LOGSTUATUS(mode) {                                                     \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
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

#define LOGAMESAVING() { \
    if (!logFile) {                                                             \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(logFile, "%s Saving the game.\n", date); \
    fflush(logFile); \
}

#define LOGAMESAVED() { \
    if (!logFile) {                                                             \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(logFile, "%s Game saved.\n", date); \
    fflush(logFile); \
}

#define LOGQUIT(){ \
    if (!logFile) {                                                             \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(logFile, "%s Quitting the game.\n", date); \
    fflush(logFile); \
    }


#define LOGNEWMAP(status) {                                                      \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
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

#define LOGENDGAME(status, inputStatus) {                                        \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(logFile, "%s Level %d not completed. Game end. Total score: %d\n",   \
            date, status.level, inputStatus.score);                              \
    fflush(logFile);                                                             \
}

#define LOGENDLEVEL(status, inputStatus) {                                       \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
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
        raise(SIGTERM);                                                                  \
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
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(logFile, "%s Process %d dead\n",   \
            date, pid);                              \
    fflush(logFile);                                                             \
} 

#define LOGBBDIED() { \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                           \
    fprintf(logFile, "%s Blackboard is quitting\n", date);                              \
    fflush(logFile); \
}

#if USE_DEBUG
#define LOGDRONEINFO(droneInfo) { \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
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

#if USE_DEBUG
#define LOGPROCESSELECTED(selected) {   \
    if (!logFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                            \
    char process[20]; \
    switch (selected) { \
        case DRONE: \
            strcpy(process, "Drone"); \
            break; \
        case INPUT: \
            strcpy(process, "Input"); \
            break; \
        case OBSTACLE: \
            strcpy(process, "Obstacle or Target"); \
            break; \
        default: \
            strcpy(process, "Not valid"); \
        }                                                                       \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(logFile, "%s Process selected: %s", date, process); \
}
#else
#define LOGPROCESSELECTED(selected) {}
#endif

#if USE_DEBUG
#define LOGINPUTMESSAGE(inputMsg) {\
    if (!logFile) { \
        perror("Log file not initialized.\n"); \
        raise(SIGTERM); \
    } \
    char date[50]; \
    getFormattedTime(date, sizeof(date)); \
    fprintf(logFile, "%s Direction: %s.\n", date, inputMsg.input); \
    fflush(logFile); \
}
#else
#define LOGINPUTMESSAGE(inputMsg) {}
#endif

#endif // LOG_H
