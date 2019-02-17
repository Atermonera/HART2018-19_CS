#include "datagen.h"

// Sets values to axis variables
void set_axis(struct axis* a, dist, vel, accel){
	a->dista = dist;
	a->veloc = vel;
	a->accel = accel;
	return;
}

/* Function: get_temperature
 * Description: Determines approximate temperature for a given altitude, 
 *		given a base altitude and temperature
 * Parameters:
	* altitude - altitude in feet above ground level
	* temp - temperature at ground level
 * Returns:
	Temperature at the specified altitude
 */
double get_temperature(double altitude, double temp){
	for(int i = 0; i < altitude; i++){
		if(i < 36000)		// Trposphere
			temp += -1.98 / 1000;
		else if(i < 65000)	// Tropopause
			continue;
		else if(i < 105000)	// Stratosphere 1
			temp += 0.3 / 1000;
		else if(i < 154200) // Stratosphere 2
			temp += 0.8537 / 1000;
		else if(i < 167300)	// Stratopause
			continue;
		else				// Mesosphere
			temp += -0.8214 / 1000;
	}
	return temp;
}

int gen_data(){
	
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
	
	struct dataset* data = malloc(sizeof(struct dataset) * (int) (measurement_duration / measurement_delta));
	double clk;
	
	// TODO: Actually make lateral translations
	// TODO: Use temperature
	// TODO: Figure out how the gyro measures orientation
	
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
	int i = 0;
	int stage_count = 0;
	double torque[3];
	double term_vel = TERM_VEL;
	
	// RNG
	time_t t;
	srand((unsigned) time(&t));
	int direction = rand() % 360; // Direction of lateral translation, degrees from east
	int dir_change_timer = rand()%(int)(1/measurement_delta);
	
	// Initial step, no motion
	data[i].temperature = GROUND_TEMP; // No change to temp
	for(int j = 0; j < 2; j++)	
		for(int k = 0; k < 2; k++)
			data[i].gyro[j][k] = 0;		// No rotation
	set_axis(&(data[i].x), 0, 0, 0);	// No motion
	set_axis(&(data[i].y), 0, 0, 0);
	set_axis(&(data[i].z), 0, 0, 0);
	
	clk += measurement_delta;
	i++;
	
	while(clk < measurement_duration){
		char buffer[256];
		memset(buffer, '\0', sizeof(buffer));
		
		// Increment stage count if it's passed the timer
		if(launch_profile[stage_count][0] > 0 && clk < launch_profile[stage_count][0])
			stage_count++;
		
		
		// Rotation
		if(data[i-1].y.dista != 0.0)
			torque[GYRO_Y] = data[i-1].y.veloc / data[i-1].y.dista; // Moving faster at lower altitudes causes the most rotation
		else
			torque[GYRO_Y] = 0;
	//	torque[GYRO_X] = MIN(sin(direction * PI / 180) / data[i-1].y.veloc, 1);
	//	torque[GYRO_Z] = MIN(cos(direction * PI / 180) / data[i-1].y.veloc, 1);
		for(int j = 0; j < 2; j++)
			for(int k = 0; k < 2; k++)
				data[i].gyro[j][k] = data[i-1].gyro[j][k];
		
		update_alignment(data[i].gyro, torque);
		
		
		// Update vertical acceleration
		data[i].y.accel = launch_profile[stage_count][1];
		if(launch_profile[stage_count][2])
			data[i].y.accel += (data[i].y.veloc < 0 ? 1 : -1) *(pow(y_vel, 2) * 32.17405) / (y_dist * pow(term_vel, 2.0);
		
		// TODO: convert fixed-axis accel into variable-axis accel
		
		// Vertical motion
		data[i].y.veloc = data[i-1].y.veloc + measurement_delta * data[i].y.accel;
		data[i].y.dista = data[i-1].y.dista + measurement_delta * data[i].y.veloc;
		
		data[i].temperature = get_temperature(data[i].y.dista, GROUND_TEMP);
		
		// TODO: lateral translation
		set_axis(&(data[i].x), 0, 0, 0);
		set_axis(&(data[i].z), 0, 0, 0);
		
		// Can't fall through the earth
		if(data[i].y.dista){
			set_axis(&(data[i].z), 0, 0, 0);
		}
	

		// Update terminal velocity
		if(data[i].y.accel > 1500)
			term_vel = TERM_VEL;
		else if(data[i].y.accel > 500)
			term_vel = DROG_TERM_VEL;
		else
			term_vel = MAIN_TERM_VEL;
		
		
		data[i].clk = clk;
		// Increment clock to next measurement
		clk += measurement_delta;
		i++;
	}
	
	close(file);
	
	return 0;
}