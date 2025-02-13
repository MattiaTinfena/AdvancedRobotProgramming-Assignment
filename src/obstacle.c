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
#include "obstacle.h"

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2


#define OBSTACLE_MASS 0.1

int pid;
int fds[4];

Message status;


FILE *obstFile = NULL;

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

int main(int argc, char *argv[]) {
    fdsRead(argc, argv, fds);

    // Opening log file
    obstFile = fopen("log/obstacle.log", "a");
    if (obstFile == NULL) {
        perror("Errore nell'apertura del obstFile");
        exit(1);
    }

    pid = writePid("log/passParam.txt", 'a', 1, 'o');

    // Closing unused pipes heads to avoid deadlock
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

    

    for( int i = 0; i < MAX_OBSTACLES; i++){
        status.obstacles.x[i] = 0;
        status.obstacles.y[i] = 0;
    }
    
    status.obstacles.incr = 0;

    while (1) {

        // Read drone position
        readMsg(fds[recrd], &status,
            "[OBSTACLE] Error reading drone and target position from [BB]", obstFile);
        LOGDRONEINFO(status.drone);

        createObstacles();
        LOGNEWMAP(status);
        
        // fprintf(obstFile,"obstincr: %d", status.obstacles.incr);
        // fflush(obstFile);
        writeMsg(fds[askwr], &status, 
            "[OBSTACLE] Error sending obstacle positions to [BB]", obstFile);

        usleep(100000);
    }
}
