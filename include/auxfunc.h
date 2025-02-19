#ifndef AUXFUNC_H
#define AUXFUNC_H

#include <signal.h>

#define DRONE 0        
#define INPUT 1        
#define OBSTACLE 2        
#define TARGET 3 
#define BLACKBOARD 4
#define WATCHDOG 5

#define WINDOW_WIDTH 100
#define WINDOW_LENGTH 100

#define MAX_TARGET 20
#define MAX_OBSTACLES 20
#define NO_SPAWN_DIST 5

#define MAX_LINE_LENGTH 100
#define MAX_FILE_SIZE 1024

#define PLAY 0
#define PAUSE 1

#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

extern const char *moves[9];

typedef struct {
    int x;
    int y;
    float speedX;
    float speedY;
    float forceX;
    float forceY;
} Drone_bb;

typedef struct {
    float x;
    float y;
} Force;

typedef struct {
    float x;
    float y;
} Speed;
typedef struct {
    int x[MAX_TARGET];
    int y[MAX_TARGET];
    int value[MAX_TARGET];
    int incr;
} Targets;
typedef struct
{
    int x[MAX_OBSTACLES];
    int y[MAX_OBSTACLES];
    int incr;
} Obstacles;

typedef struct {
    char msg;
    int level;
    int difficulty;
    char input[10];
    Drone_bb drone;
    Targets targets;
    Obstacles obstacles;
} Message;

typedef struct{
    char msg;
    char name[MAX_LINE_LENGTH];
    char input[10];
    int difficulty;
    int level;
    int score;
    Drone_bb droneInfo;
} inputMessage;

typedef struct {
    
    char name[MAX_LINE_LENGTH];
    int score;
    int level;
} Player;

extern char jsonBuffer[MAX_FILE_SIZE];

void handleLogFailure();
int writeSecure(char* filename, char* data, unsigned long numeroRiga, char mode);
int readSecure(char* filename, char* data, unsigned long numeroRiga);
void handler(int id);
void writeMsg(int pipeFds, Message* msg, char* error, FILE* file);
void readMsg(int pipeFds, Message* msgOut, char* error, FILE* file);
void writeInputMsg(int pipeFds, inputMessage* msg, char* error, FILE* file);
void readInputMsg(int pipeFds, inputMessage* msgOut, char* error, FILE* file);
void fdsRead (int argc, char* argv[], int* fds);
int writePid(char* file, char mode, int row, char id);
void printInputMessageToFile(FILE *file, inputMessage* msg);
void printMessageToFile(FILE *file, Message* msg);
void msgInit(Message* status);
void inputMsgInit(inputMessage* status);
void getFormattedTime(char *buffer, size_t size);

#endif