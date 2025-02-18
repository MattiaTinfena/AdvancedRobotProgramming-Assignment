#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "cjson/cJSON.h"
#include "auxfunc.h"
#include "obstacle.h"

#define PERIODO 100000
#define OBSTACLE_MASS 0.1

FILE *obstFile = NULL;
FILE *settingsfile = NULL;

Message status;

int pid;
int fds[4];

int numTarget = 5;
int numObstacle = 5;

int main(int argc, char *argv[]) {
    
    fdsRead(argc, argv, fds);

    // Opening log file
    obstFile = fopen("log/obstacle.log", "a");
    if (obstFile == NULL) {
        perror("Errore nell'apertura del obstFile");
        exit(1);
    }

    pid = writePid("log/passParam.txt", 'a', 1, 'o');

    //Open config file
    settingsfile = fopen("appsettings.json", "r");
    if (settingsfile == NULL) {
        perror("Error opening the file");
        return EXIT_FAILURE;
    }

    // Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    //Defining signals
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags = SA_RESTART;  

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    readConfig();

    for( int i = 0; i < MAX_OBSTACLES; i++){
        status.obstacles.x[i] = 0;
        status.obstacles.y[i] = 0;
    }
    
    status.obstacles.incr = 0;

    while (1) {

        readMsg(fds[recrd], &status,
            "[OBSTACLE] Error reading drone and target position from [BB]", obstFile);
        LOGDRONEINFO(status.drone);

        createObstacles();
        LOGNEWMAP(status);
        
        writeMsg(fds[askwr], &status, 
            "[OBSTACLE] Error sending obstacle positions to [BB]", obstFile);

        usleep(PERIODO);
    }
}

/*********************************************************************************************************************/
/************************************FUNCTIONS TO CREATE NEW OBSTACLES************************************************/
/*********************************************************************************************************************/

int canSpawn(int x_pos, int y_pos) {
    for (int i = 0; i < numTarget + status.targets.incr; i++) {
        if (abs(x_pos - status.targets.x[i]) <= NO_SPAWN_DIST && abs(y_pos - status.targets.y[i]) <= NO_SPAWN_DIST) return 0; 
    }
    return 1;
}

int canSpawnPrev(int x_pos, int y_pos) {
    for (int i = 0; i < numObstacle + status.obstacles.incr; i++) {
        if (abs(x_pos - status.obstacles.x[i]) <= NO_SPAWN_DIST && abs(y_pos - status.obstacles.y[i]) <= NO_SPAWN_DIST) return 0;
    }
    return 1;
}

void createObstacles() {

    // This function creates new obstacles ensuring that they do not overlap with the drone, the targets and the old obstacles.
    int x_pos, y_pos;

    for( int i = 0; i < MAX_OBSTACLES; i++){
        status.obstacles.x[i] = 0;
        status.obstacles.y[i] = 0;
    }
    
    for (int i = 0; i < numObstacle + status.obstacles.incr; i++){
    
        do {
            x_pos = rand() % (WINDOW_LENGTH - 1);
            y_pos = rand() % (WINDOW_WIDTH - 1);
        } while (
            ((abs(x_pos - status.drone.x) <= NO_SPAWN_DIST) &&
            (abs(y_pos - status.drone.y) <= NO_SPAWN_DIST)) || 
            canSpawn(x_pos, y_pos) == 0 ||
            canSpawnPrev(x_pos, y_pos) == 0);

        status.obstacles.x[i] = x_pos;
        status.obstacles.y[i] = y_pos;
    }
}

/*********************************************************************************************************************/
/***********************************************SIGNAL HANDLER********************************************************/
/*********************************************************************************************************************/


void sig_handler(int signo) {
    if (signo == SIGUSR1)
    {
        handler(OBSTACLE);
    }else if(signo == SIGTERM){
        LOGPROCESSDIED() 
        fclose(obstFile);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
    }
}

/*********************************************************************************************************************/
/********************************************READING CONFIGURATION****************************************************/
/*********************************************************************************************************************/

void readConfig() {

    int len = fread(jsonBuffer, 1, sizeof(jsonBuffer), settingsfile); 
    if (len <= 0) {
        perror("Error reading the file");
        return;
    }
    fclose(settingsfile);

    cJSON *json = cJSON_Parse(jsonBuffer);

    if (json == NULL) {
        perror("Error parsing the file");
    }

    numTarget = cJSON_GetObjectItemCaseSensitive(json, "TargetNumber")->valueint;
    numObstacle = cJSON_GetObjectItemCaseSensitive(json, "ObstacleNumber")->valueint;

    cJSON_Delete(json);
}