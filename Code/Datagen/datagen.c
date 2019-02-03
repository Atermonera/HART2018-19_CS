#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#define GRAVITATION -32.17405					// Standard constant of gravition in ft/s^2
#define BOOSTER_ACCELERATION 32.17405 *   11.2 	// ~11.2G's acceleration
#define SUSTAINER_ACCELERATION 32.17405 * 22 	// ~22G's acceleration
#define TERM_VEL 250 							// 250fts terminal velocity
#define DROG_TERM_VEL 80 						// 80ft/s terminal velocity under drogue
#define MAIN_TERM_VEL 10 						// 10ft/s terminal velocity under main


int main(){
	
	/********
	Axes:
	^ Upwards
	
		Y	
		|  /
		| /
		|/
	----*----X
	   /|
	  / |
	 /  |
	Z
	********/
	
	double x_accel, x_vel, x_dist;
	double y_accel, y_vel, y_dist;
	double z_accel, z_vel, z_dist;
	
	double temperature;
	// TODO: Actually make lateral translations
	// TODO: Use temperature
	// TODO: Figure out how the gyro measures orientation
	
	double measurement_delta = 0.01; // Arbitrary: 100 measurement cycles per second
	double clk;
	int measurement_duration = 600; // 10m flight period in seconds
	
	// Open the output file
	char outfile[50]; // File path
	memset(outfile, '\0', sizeof(outfile));
	time_t rawtime = time(NULL);
	struct tm *timeinfo = localtime(&rawtime);
	strftime(outfile, sizeof(outfile), "%F-%T.JSON", timeinfo);
	int file = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	// Piecewise generation of data is as follows
	/***********************************************************************************************************
	{T=0,  T=5: Motionless on pad																				}
	{T=5,  T=8: First stage fires, constant positive acceleration												}
	{T=8,  T=14: Interstage coast, constant negative acceleration												}
	{T=15, T=24: Second stage fires, constant positive acceleration												}
	{T=24, T=INF: Piecewise generation by altitude, velocity													}
		{Constant negative acceleration																			}
		{Negative velocity: Exponential decay of accel until terminal velocity									}
		{Negative velocity, 1500 altitude: Drogue chute deploys, accel until new terminal velocity				}
		{Negative velocity, 500 altitude: Main chute deploys, accel until new terminal velocity or altitude 0	}
		{0 altitude: Landed, motionless for remaining duration													}
	************************************************************************************************************/
	
	clk = 0.0;
	while(clk < measurement_duration){
		char buffer[256];
		memset(buffer, '\0', sizeof(buffer));
		// Stage 1: Motionless on pad
		if(clk < 5){
			sprintf(buffer, "{clock: %f, y_dist: 0, y_vel: 0, y_accel: 0}\n", clk);
			write(file, buffer, strlen(buffer));
		}
		
		// Stage 2: Booster engine fires, positive acceleration
		else if(clk < 11){
			y_accel = BOOSTER_ACCELERATION;
			y_vel += y_accel * measurement_delta;
			y_dist += y_vel * measurement_delta;
			
			sprintf(buffer, "{clock: %f, y_dist: %f, y_vel: %f, y_accel: %f}\n", clk, y_dist, y_vel, y_accel);
			write(file, buffer, strlen(buffer));
		}
		
		// Stage 3: Interstage coast, negative acceleration
		else if(clk < 16){
			y_accel = GRAVITATION;
			y_vel += y_accel * measurement_delta;
			y_dist += y_vel * measurement_delta;
			
			sprintf(buffer, "{clock: %f, y_dist: %f, y_vel: %f, y_accel: %f}\n", clk, y_dist, y_vel, y_accel);
			write(file, buffer, strlen(buffer));
		}
		
		// Stage 4: Sustainer engine fires, positive acceleration
		else if(clk < 22){
			y_accel = SUSTAINER_ACCELERATION;
			y_vel += y_accel * measurement_delta;
			y_dist += y_vel * measurement_delta;
			
			sprintf(buffer, "{clock: %f, y_dist: %f, y_vel: %f, y_accel: %f}\n", clk, y_dist, y_vel, y_accel);
			write(file, buffer, strlen(buffer));
		}
		
		// Stage 5: Coasting to apogee, negative acceleration
		else if(y_vel > 0){
			y_accel = GRAVITATION - (pow(y_vel, 2) * 32.17405) / (y_dist * pow(TERM_VEL, 2.0) /2600);
			y_vel += y_accel * measurement_delta;
			y_dist += y_vel * measurement_delta;
			
			sprintf(buffer, "{clock: %f, y_dist: %f, y_vel: %f, y_accel: %f}\n", clk, y_dist, y_vel, y_accel);
			write(file, buffer, strlen(buffer));
		}
		
		// Stage 6: Falling from apogee, negative acceleration until at terminal velocity
		else if(y_dist > 1500){
			// Terminal velocity: 80
			// positive coast acceleration, because that already has the negative
			// Drag = const * vel^2
			// term_vel = sqrt(Weight/const)
			// Drag = vel^2 / term_vel^2
			y_accel = ((pow(y_vel, 2) * 32.17405) / (y_dist * pow(TERM_VEL, 2.0) /2600)) + GRAVITATION;
			y_vel += y_accel * measurement_delta;
			y_dist += y_vel * measurement_delta;
			
			sprintf(buffer, "{clock: %f, y_dist: %f, y_vel: %f, y_accel: %f}\n", clk, y_dist, y_vel, y_accel);
			write(file, buffer, strlen(buffer));
		}
		
		// Stage 7: Falling under drogue, positive acceleration to new terminal velocity
		else if(y_dist > 500){
			// Terminal velocity: 25
			// positive coast acceleration, because that already has the negative
			// Drag = const * vel^2
			// term_vel = sqrt(Weight/const)
			// Drag = vel^2 / term_vel^2
			y_accel = (pow(y_vel, 2) * 32.17405 / (y_dist * pow(DROG_TERM_VEL, 2.0) /2600)) + GRAVITATION;
			y_vel += y_accel * measurement_delta;
			y_dist += y_vel * measurement_delta;
			
			sprintf(buffer, "{clock: %f, y_dist: %f, y_vel: %f, y_accel: %f}\n", clk, y_dist, y_vel, y_accel);
			write(file, buffer, strlen(buffer));
		}
		
		// Stage 8: Falling under main, positive acceleration to new terminal velocity
		else if (y_dist > 0.0){
			// Terminal velocity: 3
			// positive coast acceleration, because that already has the negative
			// Drag = const * vel^2
			// term_vel = sqrt(Weight/const)
			// Drag = vel^2 / term_vel^2
			y_accel = (pow(y_vel, 2) * 32.17405 / (y_dist * pow(MAIN_TERM_VEL, 2.0) /2600)) + GRAVITATION;
			y_vel += y_accel * measurement_delta;
			y_dist += y_vel * measurement_delta;
			
			sprintf(buffer, "{clock: %f, y_dist: %f, y_vel: %f, y_accel: %f}\n", clk, y_dist, y_vel, y_accel);
			write(file, buffer, strlen(buffer));
		}
		
		// Stage 9: Landed, motionless on ground
		else{
			sprintf(buffer, "{clock: %f, y_dist: 0, y_vel: 0, y_accel: 0}\n", clk);
			write(file, buffer, strlen(buffer));

		}
		
		// Increment clock to next measurement
		clk += measurement_delta;
	}
	
	close(file);
	
	return 0;
}