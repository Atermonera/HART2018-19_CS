#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>



struct axis{
	double dista;
	double veloc;
	double accel;
};

// Read the bitstream into a packet
int receive_packet(){
	
}

// Parse the packet for the timestamp, so we know when this was sent
double parse_time(){
	
}

// Read packet, parse into variables
void parse_packet(int packet, struct axis* x, struct axis* y, struct axis*z, double* temperature, int* timestamp){
	
}


int main(){
	struct axis x;
	struct axis y;
	struct axis z;
	double temperature;
	double thrust;
	double mass;
	double magnetometer; // May be unnecessary
	int timestamp;
	int stop = 0; // Becomes true when we read a stop signal.
	
	
	char outfile[50]; // File path
	memset(outfile, '\0', sizeof(outfile));
	time_t rawtime = time(NULL);
	struct tm *timeinfo = localtime(&rawtime);
	strftime(outfile, sizeof(outfile), "%F-%T.JSON", timeinfo);
	int file = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	do{
		// Read in data
		int packet = receive_packet();
		// Check status flags (no gps, staging event, etc)
		
		// Compare timestamp
		int newtime = parse_time();
		
		// If staging, output special line
		
		// else, parse for variables
		parse_packet(packet, &x, &y, &z, &temperature, &timestamp);
		
		// Write variables to file
		
		
	} while(!stop); // When we read a stop signal, no more data
	
	close(file);
	return 0;
}



