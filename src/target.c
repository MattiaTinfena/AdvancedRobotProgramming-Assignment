#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "auxfunc.h"
#include <signal.h>
#include <math.h>
#include "target.h"
#include "cjson/cJSON.h"

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

// management target
#define PERIODT 100000

int pid;
int fds[4]; 

FILE *targFile = NULL;
FILE *settingsfile = NULL;

Message status;

int numTarget = 5;
int numObstacle = 5;

int canSpawnPrev(int x_pos, int y_pos) {
    for (int i = 0; i < numTarget + status.targets.incr; i++) {
        if (abs(x_pos - status.targets.x[i]) <= NO_SPAWN_DIST && abs(y_pos - status.targets.y[i]) <= NO_SPAWN_DIST) return 0;
    }
    return 1;
}

void createTargets() {
    int x_pos, y_pos;

    for (int i = 0; i < numTarget + status.targets.incr; i++)
    {
        if(status.targets.value[i] != 0){
            do {
                x_pos = rand() % (WINDOW_LENGTH - 1);
                y_pos = rand() % (WINDOW_WIDTH - 1);
            } while (
                ((abs(x_pos - status.drone.x) <= NO_SPAWN_DIST) &&
                (abs(y_pos - status.drone.y) <= NO_SPAWN_DIST)) || 
                canSpawnPrev(x_pos, y_pos) == 0);

            status.targets.x[i] = x_pos;
            status.targets.y[i] = y_pos;
        }
    }
}

void refreshMap(){

    status.msg = 'R';

    // send drone position to target
    writeMsg(fds[askwr], &status, 
            "[TARGET] Ready not sended correctly", targFile);


    status.msg = '\0';

    readMsg(fds[recrd], &status,
            "[TARGET] Error reading drone position from [BB]", targFile);
    LOGDRONEINFO(status.drone);


    createTargets();             // Create target vector
    LOGNEWMAP(status);


    writeMsg(fds[askwr], &status, 
            "[TARGET] Error sending target position to [BB]", targFile);
}

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(TARGET);
    }else if(signo == SIGTERM){
        LOGPROCESSDIED(); 
        fclose(targFile);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
    }
}

void readConfig() {

    int len = fread(jsonBuffer, 1, sizeof(jsonBuffer), settingsfile); 
    if (len <= 0) {
        perror("Error reading the file");
        return;
    }
    fclose(settingsfile);

    cJSON *json = cJSON_Parse(jsonBuffer); // parse the text to json object

    if (json == NULL) {
        perror("Error parsing the file");
    }

    // Aggiorna le variabili globali
    numTarget = cJSON_GetObjectItemCaseSensitive(json, "TargetNumber")->valueint;
    numObstacle = cJSON_GetObjectItemCaseSensitive(json, "ObstacleNumber")->valueint;

    cJSON_Delete(json); // pulisci
}

int main(int argc, char *argv[]) {

    fdsRead(argc, argv, fds);
    
    // Opening log file
    targFile = fopen("log/target.log", "a");
    if (targFile == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    pid = writePid("log/passParam.txt", 'a', 1, 't');

    //Open config file
    settingsfile = fopen("appsettings.json", "r");
    if (settingsfile == NULL) {
        perror("Error opening the file");
        return EXIT_FAILURE;//1
    }

    //Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    //Defining signals
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags = SA_RESTART;  // Riavvia read/write interrotte
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    readConfig();

    readMsg(fds[recrd], &status,
            "[TARGET] Error reading drone position from [BB]", targFile);

    LOGDRONEINFO(status.drone);

    createTargets();             // Create target vector
    LOGNEWMAP(status);

    writeMsg(fds[askwr], &status, 
            "[TARGET] Error sending target position to [BB]", targFile);
    

    while (1) {

        refreshMap();
        usleep(PERIODT);
    }
}
