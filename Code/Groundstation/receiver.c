#include "serial_telemetry.h"
#include <math.h>
#include <time.h>
#include <sys/stat.h>

struct axis{
	double dista;
	double veloc;
	double accel;
};

int main(){
	struct axis x;
	struct axis y;
	struct axis z;
	double rot[3];
	double temperature;
	double thrust;
	double mass;
	double start_lat = -1; // Starting latitude and longitude
	double start_lon = -1; // for distance calculations
	int gps_set = 0; // If we've seen a gps packet for this data packet.
	int stop = 0; // Becomes true when we read a stop signal.
	int last_time = -1;
	char* buffer = malloc(sizeof(char) * 1024);
	memset(buffer, '\0', sizeof(buffer));
	
	
	char outfile[50]; // File path
	memset(outfile, '\0', sizeof(outfile));
	time_t rawtime = time(NULL);
	struct tm *timeinfo = localtime(&rawtime);
	strftime(outfile, sizeof(outfile), "%F-%Hh-%Mm-%Ss.JSON", timeinfo);
	int file = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
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
			write(file, buffer, strlen(buffer));
			continue;
		}
		// Compare timestamp
		if(last_time > timestamp.time){
			sprintf(buffer, "ERROR: Timestamp out of order. Received %d, expected no lower than %d\n", timestamp.time, last_time);
			write(file, buffer, strlen(buffer));
			continue;	// No use trying to interpolate with a previous state as current
		} else
			last_time = timestamp.time;
		
		// If staging, output special line
		if(timestamp.code & 0x8){
			sprintf(buffer, "EVENT: LAUNCH\n");
			write(file, buffer, strlen(buffer));
		}
		
		if(timestamp.code & 0x10){
			sprintf(buffer, "EVENT: STAGE SEPARATION");
			write(file, buffer, strlen(buffer));
		}
		
		if(timestamp.code & 0x20){
			sprintf(buffer, "EVENT: DROGUE DEPLOYMENT");
			write(file, buffer, strlen(buffer));
		}
		
		if(timestamp.code & 0x40){
			sprintf(buffer, "EVENT: MAIN DEPLOYMENT");
			write(file, buffer, strlen(buffer));
		}
		
		// else, parse for variables
		if(stat == 0){			// Non-GPS data
			x.accel = packet.acc.x;
			y.accel = packet.acc.y;
			z.accel = packet.acc.z;
			temperature = packet.tmp;
			rot[0] = packet.gyro.x;
			rot[1] = packet.gyro.y;
			rot[2] = packet.gyro.z;
			y.dista = 145366.45 * (1 - pow(packet.prs/101325, 0.190284)); //Convert pressure in Pascals to Pressure Altitude (ft) according to ISA
			sprintf(buffer, "PKT: x_acc %f, y_acc %f, z_acc %f, y_alt %f, tmptr %f, x_rot %f, y_rot %f, z_rot %f\n", x.accel, y.accel, z.accel, y.dista, temperature, rot[0], rot[1], rot[2]);
		} else if (stat == 1){	// GPS data
			gps_set = 1;
			// Store the first gps coordinates as the origin for distance measurements.
			if(start_lat == -1)
				start_lat = gps.latitude;
			if(start_lon == -1)
				start_lon = gps.longitude;
			sprintf(buffer, "GPS: lat %f, lon %f, spd %f, ang %f, alt %f\n", gps.latitude, gps.longitude, gps.speed, gps.angle, gps.altitude);
		}
		
		// Write variables to file
		write(file, buffer, strlen(buffer));
		
		// Stop logic
			// Never stop
	} while(!stop); // When we read a stop signal, no more data
	
	close(file);
	return 0;
}