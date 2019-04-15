#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>



#ifndef SERIAL_TELEMETRY_H
#define SERIAL_TELEMETRY_H


struct gpsData{
  float latitude;
  float longitude;
  float speed;
  float angle;
  float altitude;
  char misc;

  ///////////////////
  // code for misc //
  ///////////////////

  // bit 0 -> lat
  // bit 1 -> lon

}data_gps;

struct xyzData{
  float x;
  float y;
  float z;
};

struct dataPoint { 
  struct xyzData acc;   // m/s^2  //might be able to get higher resolution by raw data instead
  struct xyzData gyro;  // rad/s
  float prs;            // Pa
  float tmp;            // C
}data_telemetry; 

struct basic{
  unsigned int time;    // ms
  unsigned int code;
  ///////////////////
  // code for code //
  ///////////////////

  // bit 0 -> 0 if dataPoint, 1 if dataGPS
  // bit 1 -> 0 -> no gps fix
  // bit 2 -> Address -> 0 if booster, 1 if sustainer
  // bit 3 -> event 0 -> 1 if launch happened
  // bit 4 -> event 1 -> 1 if separation/sustainer happened
  // bit 5 -> event 2 -> 1 if drouge shoot happened
  // bit 6 -> event 3 -> 1 if main shoot happened
  // bit 7 -> 1 if sending battery info // maybe implement

  // INT IS 16 BITS YOU CAN ADD MORE CODES

  // bool mybit = ((code >> 3)  & 0x01) //stores bit 3 of "code" in mybit
}time_code; 

#endif

int set_interface_attribs(int fd, int speed);
void set_mincount(int fd, int mcount);
int setup_serial(char* portname); //returns file descriptor, -1 if error
int gather_telemetry(int fd, struct basic *time_code, struct dataPoint *data_telemetry, struct gpsData *data_gps);

