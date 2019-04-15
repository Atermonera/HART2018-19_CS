#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

#include "serial_telemetry.h"

// Serial code based off of 
// https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c


int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

int setup_serial(char * portname){
    int fd;

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);  
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    set_interface_attribs(fd, B115200);
    return fd;
    //set_mincount(fd, 0);                /* set to pure timed read */
}


int gather_telemetry(int fd, struct basic * time_code, struct dataPoint * data_telemetry, struct gpsData * data_gps)
{
    unsigned char buf[60];
    int rdlen;

    rdlen = read(fd, buf, sizeof(buf)); //originally was rdlen = read(fd, buf, sizeof(buf)-1);
    if (rdlen > 0) {
        //usleep(1000);
        memcpy(time_code, buf, sizeof(*time_code)); // copy first part of buffer to "basic" struct
        

        if(!(time_code->code & (1 << 0))){ // if reading dataPoint
            memcpy(data_telemetry, &buf[sizeof(*time_code)], sizeof(*data_telemetry)); // copy second part to "dataPoint" struct
            return 0;
        }

        else{ // if reading gpsData
            memcpy(data_gps, &buf[sizeof(*time_code)], sizeof(*data_gps)); // copy second part of buf to gpsData struct
            return 1;
        }
    } 
    else if (rdlen < 0) {
        printf("Error from read: %d: %s\n", rdlen, strerror(errno));
        return -1;
    } 
    else {   //rdlen == 0 
        printf("Timeout from read\n");
        return -1;
    }
}