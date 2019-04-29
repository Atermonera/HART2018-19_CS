#include "serial_telemetry.h"
#include <math.h>
#include <time.h>
#include <sys/stat.h>

#define CONTENTS 0x01
#define GPS_FIX 0x02
#define ADDRESS 0x04
#define IGNITION 0x08
#define SEPARATION 0x10
#define DROGUE 0x20
#define MAIN 0x40

struct axis{
	double dista;
	double veloc;
	double accel;
};

struct stage{
	struct axis x;
	struct axis y;
	struct axis z;
	struct gpsData gps;	// Last GPS info seen
	double rot[3];
	double temperature;
	double thrust;
	double mass;
	int gps_set;	// If we've seen a gps packet for this data packet.
	int last_time;	// Becomes true when we read a stop signal.
};

void init_stage(struct stage* S){
	S->x.dista = 0;
	S->x.veloc = 0;
	S->x.accel = 0;
	
	S->y.dista = 0;
	S->y.veloc = 0;
	S->y.accel = 0;
	
	S->z.dista = 0;
	S->z.veloc = 0;
	S->z.accel = 0;
	
	S->gps.latitude = -1;
	S->gps.longitude = -1;
	S->gps.speed = 0;
	S->gps.angle = 0;
	S->gps.altitude = 0;
	S->gps.misc = '\0';
	
	S->temperature = 293.15; // Room temperature, have to set it somewhere
	S->rot[0] = 0;
	S->rot[1] = 0;
	S->rot[2] = 0;
	S->thrust = -1;
	S->mass = -1;
	S-?gps_set = 0;
	S->last_time = -1;
	
	return;
}

int main(){
	struct stage booster;
	struct stage sustainer;
	struct stage* curr;
	int* curr_out;
	int* curr_log;
	
	init_stage(&booster);
	init_stage(&sustainer);
	
	double start_lat = -1; // Starting latitude and longitude
	int stop = 0; 
	char* buffer = malloc(sizeof(char) * 1024);
	memset(buffer, '\0', sizeof(buffer));
	
	
	char outfile[50]; // File path
	memset(outfile, '\0', sizeof(outfile));
	sprintf(outfile, "bst_out.csv");
	int bst = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	memset(outfile, '\0', sizeof(outfile));
	sprintf(outfile, "bst.log");
	int bstlog = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	memset(outfile, '\0', sizeof(outfile));
	sprintf(outfile, "sst_out.csv");
	int sst = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	memset(outfile, '\0', sizeof(outfile));
	sprintf(outfile, "sst.log");
	int sstlog = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	int stat;						// Stores return value of gather_telemetry
	char* port = "/dev/ttyACM0";	// Location of USB0 pipe on Raspi
	int pipe = setup_serial(port);	// Open file descriptor to USB0 pipe
	struct basic timestamp;
	struct dataPoint packet;
	struct gpsData gps;
	
	do{
		memset(buffer, '\0', sizeof(buffer));
		// Read in data
		stat = gather_telemetry(pipe, &timestamp, &packet, &gps);
		
		// Check status flags (no gps, staging event, etc)
		if(stat != 0 && stat != 1){// gather_telemetry failed
			sprintf(buffer, "ERROR: Failed to read packet\n");
			write(sstlog, buffer, strlen(buffer));
			write(bstlog, buffer, strlen(buffer));
			continue;
		}
		
		// Which stage we've received a packet for.
		if(timestamp.code & ADDRESS){
			curr = &sustainer;
			curr_out = &sst;
			curr_log = &sstlog;
		}else{
			curr = &booster;
			curr_out = &bst;
			curr_log = &bstlog;
		}
		// Compare timestamp
		if(curr->last_time > timestamp.time){
			sprintf(buffer, "ERROR: Timestamp out of order. Received %d, expected no lower than %d\n", timestamp.time, last_time);
			write(*currlog, buffer, strlen(buffer));
			continue;	// No use trying to interpolate with a previous state as current
		} else
			curr->last_time = timestamp.time;
		
		// If staging, output special line
		if(timestamp.code & 0x8){
			sprintf(buffer, "EVENT: LAUNCH\n");
			write(curr_log, buffer, strlen(buffer));
		}
		
		if(timestamp.code & 0x10){
			sprintf(buffer, "EVENT: STAGE SEPARATION");
			write(curr_log, buffer, strlen(buffer));
		}
		
		if(timestamp.code & 0x20){
			sprintf(buffer, "EVENT: DROGUE DEPLOYMENT");
			write(curr_log, buffer, strlen(buffer));
		}
		
		if(timestamp.code & 0x40){
			sprintf(buffer, "EVENT: MAIN DEPLOYMENT");
			write(curr_log, buffer, strlen(buffer));
		}
		
		// else, parse for variables
		if(stat == 0){			// Non-GPS data
			curr->gps_set = 0;
			curr->x.accel = packet.acc.x;
			curr->y.accel = packet.acc.y;
			curr->z.accel = packet.acc.z;
			curr->temperature = packet.tmp;
			curr->rot[0] = packet.gyro.x;
			curr->rot[1] = packet.gyro.y;
			curr->rot[2] = packet.gyro.z;
			curr->y.dista = 145366.45 * (1 - pow(packet.prs/101325, 0.190284)); //Convert pressure in Pascals to Pressure Altitude (ft) according to ISA
		
			sprintf(buffer, "PKT: x_acc %f, y_acc %f, z_acc %f, y_alt %f, tmptr %f, x_rot %f, y_rot %f, z_rot %f\n", curr->x.accel, curr->y.accel, curr->z.accel, curr->y.dista, curr->temperature, curr->rot[0], curr->rot[1], curr->rot[2]);
			write(curr_log, buffer, strlen(buffer));
			memset(buffer, '\0', strlen(buffer));
			sprintf(buffer, "%f, 0, 0, %f, %f, %f, %f, 0, 0, %f, %f, 0,0,0, 0,0,0, 0,0,0, %f, %f, %f, %f, %f\n", curr->last_time, curr->x.accel, curr->y.dista, curr->gps.speed, curr->y.accel, curr->z.accel, curr->temperature, curr->rot[0], curr->rot[1], curr->rot[2], curr->gps.latitude, curr->gps.longitude);
		
		} else if (stat == 1){	// GPS data
			curr->gps_set = 1;
			// Store the first gps coordinates as the origin for distance measurements.
			if(start_lat == -1)
				start_lat = gps.latitude;
			if(start_lon == -1)
				start_lon = gps.longitude;
			sprintf(buffer, "GPS: lat %f, lon %f, spd %f, ang %f, alt %f\n", gps.latitude, gps.longitude, gps.speed, gps.angle, gps.altitude);
			write(curr_log, buffer, strlen(buffer));
		}
				
		// Stop logic
			// Never stop
	} while(!stop); // When we read a stop signal, no more data
	
	free(buffer);
	close(bst);
	close(sst);
	close(bstlog);
	close(sstlog);
	return 0;
}