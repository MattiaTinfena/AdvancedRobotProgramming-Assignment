#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc.h"

// Macro di configurazione
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define HEIGHT  50
#define WIDTH   80

#define BUTTONS     9   
#define BTNSIZEC    10
#define BTNSIZER    5
#define BTNDISTC    11
#define BTNDISTR    5
#define BTNPOSC    20
#define BTNPOSR    20

#define LEFTUP    0
#define UP        1
#define RIGHTUP   2
#define LEFT      3
#define CENTER    4
#define RIGHT     5
#define LEFTDOWN  6
#define DOWN      7
#define RIGHTDOWN 8

#define DEFAULT 0
#define MENU 1

#define CHOOSENAME 0
#define CHOOSEBUTTON 1
#define CHOOSEDIFF 2

// Variabili globali
extern FILE *inputFile;

#define LOGPROCESSDIED() { \
    if (!inputFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                           \
    fprintf(inputFile, "%s Process dead. Input is quitting\n", date);                              \
    fflush(inputFile);                                                             \
}

#define LOGDRONEINFO(dronebb){ \
    if (!inputFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(inputFile, "%s Position (%d, %d) ", date, dronebb.x, dronebb.y); \
    fprintf(inputFile, "Speed (%.2f, %.2f) ", dronebb.speedX, dronebb.speedY); \
    fprintf(inputFile, "Force (%.2f, %.2f) ", dronebb.forceX, dronebb.forceY); \
    fprintf(inputFile, "\n"); \
    fflush(inputFile); \
}

char difficultyStr[10];
#define LOGCONFIG(status) {                                                      \
    if (!inputFile) {                                                              \
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
    fprintf(inputFile, "%s Player Name: %s, Difficulty: %s, Level: %d, Score: %d\n",          \
            date, (status).name, difficultyStr, (status).level, status.score);                 \
    fflush(inputFile);                                                             \
}

#define LOGINPUTCONFIGURATION(numbersArray) {\
    if (!inputFile) {                                                             \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(inputFile, "%s Configuration. ", date); \
    \
    int arraySize = cJSON_GetArraySize(numbersArray);\
\
    for (int i = 0; i < arraySize; ++i) { \
        cJSON *element = cJSON_GetArrayItem(numbersArray, i); \
        if (cJSON_IsNumber(element)) fprintf(inputFile,"%d,", btnValues[i] = element->valueint); \
    } \
    fprintf(inputFile, "\n"); \
    fflush(inputFile); \
}

#define LOGLEADERBOARD(leaderboard) {\
    if (!inputFile) {                                                             \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(inputFile, "%s Leaderboard. \n", date); \
    for(int i = 0; i < 10; i++){ \
        fprintf(inputFile, "\t%d. %s Score: %d Level: %d\n", i + 1, leaderboard[i].name, leaderboard[i].score, leaderboard[i].level); \
    }  \
    fflush(inputFile); \
}

#if USE_DEBUG
#define LOGAMESETTINGS() {\
    if (!inputFile) {                                                             \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(inputFile, "%s ", date); \
    fprintf(inputFile, "Game settings. \n\tLevel time %d  Target N# %d, Obstacle N# %d\n", levelTime, numTarget, numObstacle); \
    fprintf(inputFile, "\tIncrement per level - time %d target %d obstacle %d\n", incTime, incTarget, incObstacle); \
    fflush(inputFile); \
}
#else
#define LOGAMESETTINGS() {}
#endif

#define LOGSTUATUS(mode) {                                                     \
    if (!inputFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
     \
    switch (mode) { \
        case PAUSE: \
            fprintf(inputFile, "%s Status: Pause.\n", date); \
            break; \
        case PLAY: \
            fprintf(inputFile, "%s Status: Play.\n", date); \
            break; \
        default: \
            fprintf(inputFile, "%s Status: Unknown.\n", date); \
    }                                                                             \
    fflush(inputFile);                                                             \
}

#define LOGAMESAVING() { \
    if (!inputFile) {                                                             \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(inputFile, "%s Saving the game.\n", date); \
    fflush(inputFile); \
}

#define LOGAMESAVED() { \
    if (!inputFile) {                                                             \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
    fprintf(inputFile, "%s Game saved.\n", date); \
    fflush(inputFile); \
}


#define LOGDIRECTION(direction) { \
    if (!inputFile) { \
        perror("Log file not initialized.\n"); \
        return; \
    } \
    char date[50]; \
    getFormattedTime(date, sizeof(date)); \
    if (strcmp(direction, "reset") != 0) fprintf(inputFile, "%s Direction: %s.\n", date, direction); \
    fflush(inputFile); \
}

#ifdef USE_DEBUG
#define LOGACK(inputStatus) { \
    if (!inputFile) {                                                             \
        perror("Log file not initialized.\n");                                   \
        return;                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date)); \
    if(inputStatus.msg == 'A') fprintf(inputFile, "%s Ack received\n", date); \
    else fprintf(inputFile, "%s Error receiving ack\n", date); \
    fflush(inputFile); \   
}
#else
#define LOGACK(inputStatus) {}
#endif

#endif // INPUT_H