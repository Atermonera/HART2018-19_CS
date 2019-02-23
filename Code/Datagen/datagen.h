#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "update_alignment.h"

#define GRAVITATION -32.17405					// Standard constant of gravition in ft/s^2
#define BOOSTER_ACCELERATION 32.17405 *   11.2 	// ~11.2G's acceleration
#define SUSTAINER_ACCELERATION 32.17405 * 22 	// ~22G's acceleration
#define TERM_VEL 250 							// 250fts terminal velocity
#define DROG_TERM_VEL 80 						// 80ft/s terminal velocity under drogue
#define MAIN_TERM_VEL 10 						// 10ft/s terminal velocity under main

#define GROUND_TEMP 311	// ~100F, roughly accurate to New Mexico in summer

#define GYRO_X 0	// Rotation about X axis
#define GYRO_Y 1	// Rotation about Y axis
#define GYRO_Z 2	// Rotation about Z axis

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

/********
Axes:
	 Upwards
			  North
		Y	Z	
		|  /
		| /
		|/
	----*----X East
	   /|
	  / |
	 /  |
	
********/

struct axis{
	double dista;
	double veloc;
	double accel;
};

struct dataset{
	double clk;
	double temperature;
	double** gyro;
	struct axis x;
	struct axis y;
	struct axis z;
};

void set_axis(struct axis* a, double dist, double vel, double accel);

double get_temperature(double altitude, double temp);

/* Function: gen_data
 * Description: Generates test data without noise, 
	approximately simulating the flight of the rocket according to the launch profile
 * Parameters:
    * launch_profile - nx3 array of integers:
	  [0] - time this stage expires
	  [1] - flat acceleration produced during this stage
	  [2] - Boolean include drag (1 True, 0 False)
	* measurement_delta - Time in seconds between timesteps
	* measurement_duration - Time in seconds for complete flight.
		If the flight has not completed by this time, data collection stops
 * Returns:
	array containing struct dataset's for each timestep
 */
struct dataset* gen_data(double** launch_profile, double measurement_delta, int measurement_duration);
	
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
