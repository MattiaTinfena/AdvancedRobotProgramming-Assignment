
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include "cjson/cJSON.h"
#include "auxfunc.h"
#include "drone.h"


#define PERIOD 10000
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

int periodms = PERIOD/10000; 

float K = 1.0;
float droneMass = 1.0;
float eta = 1.0;
float rho_0 = 1.0;
float step = 1.0;
float psi = 1.0;
float maxForce = 1.0;

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

                newDrone(&drone, &status.targets, directions, status.msg);
                droneUpdate(&drone, &speed, &force, &status);

                writeMsg(fds[askwr], &status, 
                        "[DRONE-M] Error sending drone position", droneFile);
                break;
            case 'I':

                strcpy(directions, status.input);

                newDrone(&drone, &status.targets, directions, status.msg);
                droneUpdate(&drone, &speed, &force, &status);
                LOGDRONEINFO(status.drone);

                writeMsg(fds[askwr], &status, 
                        "[DRONE-I] Error sending drone position", droneFile);

                break;
            case 'A':
                
                newDrone(&drone, &status.targets, directions, status.msg);
                droneUpdate(&drone, &speed, &force, &status);

                writeMsg(fds[askwr], &status, 
                        "[DRONE-A] Error sending drone position", droneFile);
                usleep(10000);
                break;
            default:
                perror("[DRONE-DEFAULT] Error data received");
                exit(EXIT_FAILURE);
        }

        usleep(PERIOD);
    }
}


/*********************************************************************************************************************/
/***************************************FUNCTIONS TO COMPUTE FORCES***************************************************/
/*********************************************************************************************************************/

void drone_force(char* direction) {
    
    if (strcmp(direction, "") != 0) {

        if (strcmp(direction, "right") == 0 || strcmp(direction, "upright") == 0 || strcmp(direction, "downright") == 0) {
            force_d.x += step;
        } else if (strcmp(direction, "left") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "downleft") == 0) {
            force_d.x -= step;
        } else if (strcmp(direction, "up") == 0 || strcmp(direction, "down") == 0) {
            force_d.x += 0;
        } else if (strcmp(direction, "center") == 0 ) {
            force_d.x = 0;
        }

        if (strcmp(direction, "up") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "upright") == 0) {
            force_d.y -= step;
        } else if (strcmp(direction, "down") == 0 || strcmp(direction, "downleft") == 0 || strcmp(direction, "downright") == 0) {
            force_d.y += step;
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

void obstacle_force(Drone* drone) {
    
    force_o.x = 0;
    force_o.y = 0;

    for (int i = 0; i < numObstacle + status.obstacles.incr; i++) {
        Force rep = compute_repulsive_force(drone, (float)status.obstacles.x[i], (float)status.obstacles.y[i]);
        force_o.x += rep.x;
        force_o.y += rep.y;
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
            distance = hypot(deltaX, deltaY);

            if (distance > rho_0) continue;

            float attraction = - psi * (distance - rho_0) / fmax(fabs(distance - rho_0), 1e-5); // Evita la divisione per zero
            if (attraction > maxForce) attraction = maxForce;
            force_t.x += attraction * (deltaX / distance);
            force_t.y += attraction * (deltaY / distance);
        }
    }

    // Limita le forze combinate
    force_t.x = fmin(force_t.x, maxForce);
    force_t.y = fmin(force_t.y, maxForce);
}

Force compute_repulsive_force(Drone *drone, float x, float y) {
    Force force = {0.0, 0.0}; // Inizializza la forza a zero
    double rho = sqrt(pow(drone->x - x, 2) + pow(drone->y - y, 2)); // Distanza

    if (rho > rho_0) return force; // Nessuna forza se fuori dal range

    // Calcola la direzione (gradiente normalizzato)
    double grad_x = (drone->x - x) / rho;
    double grad_y = (drone->y - y) / rho;

    // Calcola il coefficiente della forza
    double coeff = eta * (1.0 / rho - 1.0 / rho_0) * (1.0 / (rho * rho));

    // Assegna i valori della forza
    force.x = coeff * grad_x;
    force.y = coeff * grad_y;

    if(force.x > maxForce) force.x = maxForce;
    if(force.y > maxForce) force.y = maxForce;

    return force;
}

void boundary_force(Drone *drone) {
    force_b.x = 0;
    force_b.y = 0;

    float left_boundary = 0;
    float right_boundary = WINDOW_LENGTH;
    float up_boundary = 0;
    float down_boundary = WINDOW_WIDTH;

    Force repulsion_left = compute_repulsive_force(drone, left_boundary, drone->y);
    Force repulsion_right = compute_repulsive_force(drone, right_boundary, drone->y);
    Force repulsion_up = compute_repulsive_force(drone, drone->x, up_boundary);
    Force repulsion_down = compute_repulsive_force(drone, drone->x, down_boundary);

    force_b.x = repulsion_left.x + repulsion_right.x;
    force_b.y = repulsion_up.y + repulsion_down.y;
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

    float x_pos = (2*mass*p->previous_x[0] + periodms*K*p->previous_x[0] + force.x*periodms*periodms - mass * p->previous_x[1]) / (mass + periodms * K);
    float y_pos = (2*mass*p->previous_y[0] + periodms*K*p->previous_y[0] + force.y*periodms*periodms - mass * p->previous_y[1]) / (mass + periodms * K);

    p->x = x_pos;
    p->y = y_pos;

    p->previous_x[1] = p->previous_x[0]; 
    p->previous_x[0] = p->x;  
    p->previous_y[1] = p->previous_y[0];
    p->previous_y[0] = p->y;

    float speedX = (speedPrev->x + force.x/mass * (1.0f/periodms));
    float speedY = (speedPrev->y + force.y/mass * (1.0f/periodms));

    speedPrev->x = speed->x;
    speedPrev->y = speed->y;

    speed->x = speedX;
    speed->y = speedY;


}

void newDrone (Drone* drone, Targets* targets, char* directions, char inst){
    
    target_force(drone, targets);
    obstacle_force(drone);
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
    eta = cJSON_GetObjectItemCaseSensitive(json, "ETAObstacle")->valuedouble;
    rho_0 = cJSON_GetObjectItemCaseSensitive(json, "RHO0obstacle")->valuedouble;
    maxForce = cJSON_GetObjectItemCaseSensitive(json, "MAXForce")->valuedouble;
    step = cJSON_GetObjectItemCaseSensitive(json, "Step")->valuedouble;
    psi = cJSON_GetObjectItemCaseSensitive(json, "PSItarget")->valuedouble;
    
    cJSON_Delete(json);
}