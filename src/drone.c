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
#include <math.h>
#include <signal.h>

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define PERIOD 10 //50 //[Hz]
#define DRONEMASS 1

#define MAX_DIRECTIONS 80

float K = 1.0;

Force force_d = {0, 0};
Force force_o = {0, 0};
Force force_t = {0, 0};

Drone_bb drone_bb;

Force force = {0, 0};

Speed speedPrev = {0, 0};
Speed speed = {0, 0};

Targets targets;
Obstacles obstacles;
Message status;
Message msg;

int pid;
int fds[4];

FILE *file;

typedef struct
{
    float x;
    float y;
    float previous_x[2]; // 0 is one before and 1 is is two before
    float previous_y[2];

} Drone;

void updatePosition(Drone *p, Force force, int mass, Speed *speed, Speed *speedPrev, FILE* file) {

    fprintf(file, "-----------------------------------------------\n");
    fprintf(file, "Prev. pos (%f, %f) ", p->x, p->y);
    float x_pos = (2*mass*p->previous_x[0] + PERIOD*K*p->previous_x[0] + force.x*PERIOD*PERIOD - mass * p->previous_x[1]) / (mass + PERIOD * K);
    float y_pos = (2*mass*p->previous_y[0] + PERIOD*K*p->previous_y[0] + force.y*PERIOD*PERIOD - mass * p->previous_y[1]) / (mass + PERIOD * K);

    p->x = x_pos;
    p->y = y_pos;

    fprintf(file, "Updated pos (%f, %f) \n", x_pos, y_pos);
    fprintf(file, "x1 used (%f, %f), x2 used (%f, %f) Force used (%f, %f)\n", p->previous_x[0], p->previous_y[0], p->previous_x[1], p->previous_y[1], force.x, force.y);


    if (p->x < 0) p->x = 0;
    if (p->y < 0) p->y = 0;
    if (p->x >= WINDOW_LENGTH) p->x = WINDOW_LENGTH - 1;
    if (p->y >= WINDOW_WIDTH) p->y = WINDOW_WIDTH - 1;

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

    fflush(file);


}

void drone_force(Drone *p, float mass, float K, char* direction) {
    // Force force = {0, 0};

    // Calcolo delle derivate del movimento
    // float derivative1x = (p->x - p->previous_x[0]) / PERIOD;
    // float derivative2x = (p->previous_x[1] + p->x - 2 * p->previous_x[0]) / (PERIOD * PERIOD);
    // float derivative1y = (p->y - p->previous_y[0]) / PERIOD;
    // float derivative2y = (p->previous_y[1] + p->y - 2 * p->previous_y[0]) / (PERIOD * PERIOD);

    // // Forze derivate dalle equazioni del moto
    // force.x = mass * derivative2x - K * derivative1x;
    // force.y = mass * derivative2y - K * derivative1y +1;

    // Verifica della validità di direction
    
    if (strcmp(direction, "") != 0) {
        // Imposta direzione x
        if (strcmp(direction, "right") == 0 || strcmp(direction, "upright") == 0 || strcmp(direction, "downright") == 0) {
            force_d.x += STEP;
        } else if (strcmp(direction, "left") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "downleft") == 0) {
            force_d.x -= STEP;
        } else if (strcmp(direction, "up") == 0 || strcmp(direction, "down") == 0) {
            force_d.x += 0; // Nessuna forza lungo x
        } else if (strcmp(direction, "center") == 0 ) {
            force_d.x = 0;
        }

        if (strcmp(direction, "up") == 0 || strcmp(direction, "upleft") == 0 || strcmp(direction, "upright") == 0) {
            force_d.y -= STEP;
        } else if (strcmp(direction, "down") == 0 || strcmp(direction, "downleft") == 0 || strcmp(direction, "downright") == 0) {
            force_d.y += STEP;
        } else if (strcmp(direction, "left") == 0 || strcmp(direction, "right") == 0 ) {
            force_d.y += 0; // Nessuna forza lungo y
        } else if (strcmp(direction, "center") == 0 ) {
            force_d.y = 0;
        }
    } else {
        force_d.x += 0;
        force_d.y += 0;
    }

    // if (force_d.x > STEP*10) force_d.x = STEP*10;
    // if (force_d.y > STEP*10) force_d.y = STEP*10;
    // if (force_d.x < -STEP*10) force_d.x = -STEP*10;
    // if (force_d.y < -STEP*10) force_d.y = -STEP*10;

}

void obstacle_force(Drone *drone, Obstacles* obstacles, FILE* file) {
    // Force force = {0, 0};
    float deltaX, deltaY, distance, distance2, alpha, adjustedForceX, adjustedForceY;
    force_o.x = 0;
    force_o.y = 0;

    for (int i = 0; i < numObstacle + status.obstacles.incr; i++) {
        deltaX = drone->x - obstacles->x[i];
        deltaY = drone->y - obstacles->y[i];
        distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));
        // fprintf(file, "%f\t ", distance);
        // fflush(file);
        if (distance > FORCE_THRESHOLD) {
            continue; // Beyond influence radius
        }
        fprintf(file,"(added %d)\t", i);
        fflush(file);
        float repulsion =ETA * pow(((1/distance) - (1/FORCE_THRESHOLD)), 2)/distance;
        adjustedForceX = repulsion * cos(alpha);
        adjustedForceY = repulsion * sin(alpha);

        force_o.x -= adjustedForceX;
        force_o.y -= adjustedForceY;
    }
    fprintf(file,"\n");
    fflush(file);

}

void target_force(Drone *drone, Targets* targets, FILE* file) {
    
    float deltaX, deltaY, distance, distance2;
    force_t.x = 0;
    force_t.y = 0;

    for (int i = 0; i < numTarget + status.targets.incr; i++) {
        if(targets->value[i] > 0){    
            deltaX = targets->x[i] - drone->x;
            deltaY = targets->y[i] - drone->y;
            distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));

            fprintf(file, "%f\t ", distance);
            fflush(file);

            if (distance > FORCE_THRESHOLD)
                continue;
            if (distance < FORCE_THRESHOLD/2) continue;

            fprintf(file, "(added)\t");
            fflush(file);

            float attraction = ETA * pow(((1/distance) - (1/FORCE_THRESHOLD)), 2)/distance;
            force_t.x += attraction * (deltaX / distance);
            force_t.y += attraction * (deltaY / distance);
        }
    }
    fprintf(file, "\n");
    fflush(file);

}

Force total_force(Force drone, Force obstacle, Force target, FILE* file){
    Force total;
    total.x = drone.x + obstacle.x + target.x;
    total.y = drone.y + obstacle.y + target.y;

    fprintf(file, "FORCE: drone (%f, %f), obstacles (%f, %f), target (%f, %f)\n", drone.x, drone.y, obstacle.x, obstacle.y, target.x, target.y);
    fflush(file);

    return total;
}

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(DRONE);
    }else if(signo == SIGTERM){
        fprintf(file, "Drone is quitting\n");
        fflush(file);   
        fclose(file);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
    }
}

void newDrone (Drone* drone, Targets* targets, Obstacles* obstacles, char* directions, FILE* file, char inst){
    target_force(drone, targets, file);
    obstacle_force(drone, obstacles, file);
    if(inst == 'I'){
        drone_force(drone, DRONEMASS, K, directions);
    }
    force = total_force(force_d, force_o, force_t, file);

    updatePosition(drone, force, DRONEMASS, &speed,&speedPrev, file);
}

void droneUpdate(Drone* drone, Speed* speed, Force* force, Message* msg) {

    msg->drone.x = (int)round(drone->x);
    msg->drone.y = (int)round(drone->y);
    msg->drone.speedX = speed->x;
    msg->drone.speedY = speed->y;
    msg->drone.forceX = force->x;
    msg->drone.forceY = force->y;
}

void mapInit(Drone* drone, Message* status, Message* msg){

    
    msgInit(status);

    fprintf(file, "Updating drone position\n");
    fflush(file);

    droneUpdate(drone, &speed, &force, status);


    printMessageToFile(file, status);

    writeMsg(fds[askwr], status, 
            "[DRONE] Error sending drone info", file);
    
    fprintf(file, "Sent drone position\n");
    fflush(file);
    
    readMsg(fds[recrd], status,
            "[DRONE] Error receiving map from BB", file);
}

int main(int argc, char *argv[]) {
    
    fdsRead(argc, argv, fds);

    // Opening log file
    file = fopen("log/outputdrone.txt", "a");
    if (file == NULL) {
        perror("[DRONE] Error during the file opening");
        exit(EXIT_FAILURE);
    }

    pid = writePid("log/log.txt", 'a', 1, 'd');

    // Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    //Defining signals
    signal(SIGUSR1, sig_handler);
    signal(SIGTERM, sig_handler);
    
    char directions[MAX_DIRECTIONS] = {0};

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

    char data[200];

   mapInit(&drone, &status, &msg);

    while (1)
    {
        status.msg = 'R';

        fprintf(file, "Sending ready msg");
        fflush(file);

        writeMsg(fds[askwr], &status, 
            "[DRONE] Ready not sended correctly", file);

        status.msg = '\0';

        readMsg(fds[recrd], &status,
            "[DRONE] Error receiving map from BB", file);

        switch (status.msg) {
        
            case 'M':

                newDrone(&drone, &status.targets, &status.obstacles, directions,file,status.msg);
                droneUpdate(&drone, &speed, &force, &status);

                // drone sends its position to BB
                writeMsg(fds[askwr], &status, 
                        "[DRONE-M] Error sending drone position", file);
                break;
            case 'I':

                strcpy(directions, status.input);

                newDrone(&drone, &status.targets, &status.obstacles, directions,file,status.msg);
                droneUpdate(&drone, &speed, &force, &status);

                // drone sends its position to BB
                writeMsg(fds[askwr], &status, 
                        "[DRONE-I] Error sending drone position", file);

                break;
            case 'A':
                
                newDrone(&drone, &status.targets, &status.obstacles, directions,file,status.msg);
                droneUpdate(&drone, &speed, &force, &status);

                fprintf(file, "Drone updated position: %d,%d\n", status.drone.x, status.drone.y);
                fflush(file);
                
                // drone sends its position to BB
                writeMsg(fds[askwr], &status, 
                        "[DRONE-A] Error sending drone position", file);
                usleep(10000);
                break;
            default:
                perror("[DRONE-DEFAULT] Error data received");
                exit(EXIT_FAILURE);
        }

        usleep(1000000 / PERIOD);
    }
}
