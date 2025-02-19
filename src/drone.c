
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include "cjson/cJSON.h"
#include "auxfunc.h"
#include "drone.h"


#define PERIOD 10
#define MAX_DIRECTIONS 80

Force force_d = {0, 0};
Force force_o = {0, 0};
Force force_t = {0, 0};
Force force_b = {0, 0};

Force force = {0, 0};

Speed speedPrev = {0, 0};
Speed speed = {0, 0};

Targets targets;
Obstacles obstacles;
Message status;

FILE *droneFile = NULL;
FILE *settingsfile = NULL;

int pid;
int fds[4];

int numTarget = 5;
int numObstacle = 5;

float K = 1.0;
float droneMass = 1.0;

int main(int argc, char *argv[]) {
    
    fdsRead(argc, argv, fds);

    // Opening log file
    droneFile = fopen("log/drone.log", "a");
    if (droneFile == NULL) {
        perror("[DRONE] Error during the file opening");
        exit(EXIT_FAILURE);
    }

    //Open config file
    settingsfile = fopen("appsettings.json", "r");
    if (settingsfile == NULL) {
        perror("Error opening the file");
        return EXIT_FAILURE;
    }

    pid = writePid("log/passParam.txt", 'a', 1, 'd');

    // Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    //Initializing all the variables
    Drone drone = {0};

    drone.x = 10;
    drone.y = 20;
    drone.previous_x[0] = 10.0;
    drone.previous_x[1] = 10.0;
    drone.previous_y[0] = 20.0;
    drone.previous_y[1] = 20.0;

    for (int i = 0; i < MAX_TARGET; i++) {
        targets.x[i] = 0;
        targets.y[i] = 0;
        status.targets.x[i] = 0;
        status.targets.y[i] = 0;
    }
    targets.incr = 0;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles.x[i] = 0;
        obstacles.y[i] = 0;
        status.obstacles.x[i] = 0;
        status.obstacles.y[i] = 0;
    }
    obstacles.incr = 0;

    //Reading configuration from json file
    readConfig();

    //Defining signals
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Error while setting sigaction for SIGWINCH");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error while setting sigaction for SIGWINCH");
        exit(EXIT_FAILURE);
    }

    char directions[MAX_DIRECTIONS] = {0};

    mapInit(&drone, &status);
    LOGNEWMAP(status);

    while (1)
    {
        status.msg = 'R';

        writeMsg(fds[askwr], &status, 
            "[DRONE] Ready not sended correctly", droneFile);

        status.msg = '\0';

        readMsg(fds[recrd], &status,
            "[DRONE] Error receiving map from BB", droneFile);

        switch (status.msg) {
        
            case 'M':
                LOGNEWMAP(status);

                newDrone(&drone, &status.targets, &status.obstacles, directions, status.msg);
                droneUpdate(&drone, &speed, &force, &status);

                writeMsg(fds[askwr], &status, 
                        "[DRONE-M] Error sending drone position", droneFile);
                break;
            case 'I':

                strcpy(directions, status.input);

                newDrone(&drone, &status.targets, &status.obstacles, directions, status.msg);
                droneUpdate(&drone, &speed, &force, &status);
                LOGDRONEINFO(status.drone);

                writeMsg(fds[askwr], &status, 
                        "[DRONE-I] Error sending drone position", droneFile);

                break;
            case 'A':
                
                newDrone(&drone, &status.targets, &status.obstacles, directions, status.msg);
                droneUpdate(&drone, &speed, &force, &status);

                writeMsg(fds[askwr], &status, 
                        "[DRONE-A] Error sending drone position", droneFile);
                usleep(10000);
                break;
            default:
                perror("[DRONE-DEFAULT] Error data received");
                exit(EXIT_FAILURE);
        }

        usleep(1000000 / PERIOD);
    }
}


/*********************************************************************************************************************/
/***************************************FUNCTIONS TO COMPUTE FORCES***************************************************/
/*********************************************************************************************************************/

void drone_force(char* direction) {
    
    if (strcmp(direction, "") != 0) {

        if (strcmp(direction, "right") == 0 || strcmp(direction, "upright") == 0 || strcmp(direction, "downright") == 0) {
            force_d.x += STEP;
        } else if (strcmp(direction, "left") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "downleft") == 0) {
            force_d.x -= STEP;
        } else if (strcmp(direction, "up") == 0 || strcmp(direction, "down") == 0) {
            force_d.x += 0;
        } else if (strcmp(direction, "center") == 0 ) {
            force_d.x = 0;
        }

        if (strcmp(direction, "up") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "upright") == 0) {
            force_d.y -= STEP;
        } else if (strcmp(direction, "down") == 0 || strcmp(direction, "downleft") == 0 || strcmp(direction, "downright") == 0) {
            force_d.y += STEP;
        } else if (strcmp(direction, "left") == 0 || strcmp(direction, "right") == 0 ) {
            force_d.y += 0;
        } else if (strcmp(direction, "center") == 0 ) {
            force_d.y = 0;
        }
    } else {
        force_d.x += 0;
        force_d.y += 0;
    }

}

void obstacle_force(Drone *drone, Obstacles* obstacles) {
    
    float deltaX, deltaY, distance;
    force_o.x = 0;
    force_o.y = 0;

    for (int i = 0; i < numObstacle + status.obstacles.incr; i++) {
        deltaX =  obstacles->x[i] - drone->x;
        deltaY =  obstacles->y[i] - drone->y;

        distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));

        if (distance > FORCE_THRESHOLD) {
            continue;
        }
        float repulsion = ETA * (1/distance - 1/FORCE_THRESHOLD) * (1/(pow(distance, 2))) * (distance - FORCE_THRESHOLD);
        if (repulsion > MAX_FORCE) repulsion = MAX_FORCE;
        force_o.x += repulsion * (deltaX / distance);
        force_o.y += repulsion * (deltaY / distance);
    }

}

void target_force(Drone *drone, Targets* targets) {
    
    float deltaX, deltaY, distance;
    force_t.x = 0;
    force_t.y = 0;

    for (int i = 0; i < numTarget + status.targets.incr; i++) {
        if(targets->value[i] > 0){    
            deltaX = targets->x[i] - drone->x;
            deltaY = targets->y[i] - drone->y;
            distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));


            if (distance > FORCE_THRESHOLD) continue;

            float attraction = - ETA * (distance - FORCE_THRESHOLD) / fabs(distance - FORCE_THRESHOLD);
            if (attraction > MAX_FORCE) attraction = MAX_FORCE;
            force_t.x += attraction * (deltaX / distance);
            force_t.y += attraction * (deltaY / distance);
        }
    }
}

void boundary_force(Drone *drone) {
    force_b.x = 0;
    force_b.y = 0;

    float left_boundary = - drone ->x;
    float right_boundary = WINDOW_LENGTH - drone -> x;
    float up_boundary = - drone ->y;
    float down_boundary = WINDOW_WIDTH - drone ->y;

    float repulsion;

    if (left_boundary < FORCE_THRESHOLD) {
        repulsion = ETA * (1/left_boundary - 1/FORCE_THRESHOLD) * (1/(pow(left_boundary, 2))) * (left_boundary - FORCE_THRESHOLD);
        if (repulsion > MAX_FORCE) repulsion = MAX_FORCE;
        force_b.x += repulsion;
    }
    if (right_boundary < FORCE_THRESHOLD) {
        repulsion = ETA * (1/right_boundary - 1/FORCE_THRESHOLD) * (1/(pow(right_boundary, 2))) * (right_boundary - FORCE_THRESHOLD);
        if (repulsion > MAX_FORCE) repulsion = MAX_FORCE;
        force_b.x += repulsion;
    }
    if (up_boundary < FORCE_THRESHOLD) {
        repulsion = ETA * (1/up_boundary - 1/FORCE_THRESHOLD) * (1/(pow(up_boundary, 2))) * (up_boundary - FORCE_THRESHOLD);
        if (repulsion > MAX_FORCE) repulsion = MAX_FORCE;
        force_b.y += repulsion;
    }
    if (down_boundary < FORCE_THRESHOLD) {
        repulsion = ETA * (1/down_boundary - 1/FORCE_THRESHOLD) * (1/(pow(down_boundary, 2))) * (down_boundary - FORCE_THRESHOLD);
        if (repulsion > MAX_FORCE) repulsion = MAX_FORCE;
        force_b.y += repulsion;
    }

}

Force total_force(Force drone, Force obstacle, Force target, Force boundary){
    
    Force total;
    total.x = drone.x + obstacle.x + target.x + boundary.x;
    total.y = drone.y + obstacle.y + target.y + boundary.y;

    LOGFORCES(drone, target, obstacle);

    return total;
}

/*********************************************************************************************************************/
/****************************************FUNCTIONS TO MOVE DRONE******************************************************/
/*********************************************************************************************************************/


void updatePosition(Drone *p, Force force, int mass, Speed *speed, Speed *speedPrev) {

    float x_pos = (2*mass*p->previous_x[0] + PERIOD*K*p->previous_x[0] + force.x*PERIOD*PERIOD - mass * p->previous_x[1]) / (mass + PERIOD * K);
    float y_pos = (2*mass*p->previous_y[0] + PERIOD*K*p->previous_y[0] + force.y*PERIOD*PERIOD - mass * p->previous_y[1]) / (mass + PERIOD * K);

    p->x = x_pos;
    p->y = y_pos;


    // if (p->x < 0 || p->x >= WINDOW_LENGTH) force_d.x = 0;
    // if (p->y < 0 || p->y >= WINDOW_WIDTH) force_d.y = 0;

    // if (p->x < 0) p->x = 0;
    // else if (p->x >= WINDOW_LENGTH) p->x = WINDOW_LENGTH - 1;
    // if (p->y < 0) p->y = 0;
    // else if (p->y >= WINDOW_WIDTH) p->y = WINDOW_WIDTH - 1;

    p->previous_x[1] = p->previous_x[0]; 
    p->previous_x[0] = p->x;  
    p->previous_y[1] = p->previous_y[0];
    p->previous_y[0] = p->y;

    float speedX = (speedPrev->x + force.x/mass * (1.0f/PERIOD));
    float speedY = (speedPrev->y + force.y/mass * (1.0f/PERIOD));

    speedPrev->x = speed->x;
    speedPrev->y = speed->y;

    speed->x = speedX;
    speed->y = speedY;


}

void newDrone (Drone* drone, Targets* targets, Obstacles* obstacles, char* directions, char inst){
    
    target_force(drone, targets);
    obstacle_force(drone, obstacles);
    boundary_force(drone);
    if(inst == 'I'){
        drone_force(directions);
    }
    force = total_force(force_d, force_o, force_t, force_b);

    updatePosition(drone, force, droneMass, &speed,&speedPrev);
}

void droneUpdate(Drone* drone, Speed* speed, Force* force, Message* msg) {

    msg->drone.x = (int)round(drone->x);
    msg->drone.y = (int)round(drone->y);
    msg->drone.speedX = speed->x;
    msg->drone.speedY = speed->y;
    msg->drone.forceX = force->x;
    msg->drone.forceY = force->y;
}

void mapInit(Drone* drone, Message* status){

    msgInit(status);

    droneUpdate(drone, &speed, &force, status);
    LOGDRONEINFO(status->drone);

    writeMsg(fds[askwr], status, 
            "[DRONE] Error sending drone info", droneFile);
    
    readMsg(fds[recrd], status,
            "[DRONE] Error receiving map from BB", droneFile);
}

/*********************************************************************************************************************/
/***********************************************SIGNAL HANDLER********************************************************/
/*********************************************************************************************************************/

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(DRONE);
    }else if(signo == SIGTERM){
        LOGPROCESSDIED();   
        fclose(droneFile);
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

    K = cJSON_GetObjectItemCaseSensitive(json, "kDrone")->valuedouble;
    droneMass = cJSON_GetObjectItemCaseSensitive(json, "massDrone")->valuedouble;
    
    cJSON_Delete(json);
}