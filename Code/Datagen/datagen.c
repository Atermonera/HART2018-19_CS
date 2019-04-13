#include "datagen.h"

// Sets values to axis variables
void set_axis(struct axis* a, double dist, double vel, double accel){
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

struct dataset* gen_data(double** launch_profile, double measurement_delta, int measurement_duration){
	
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
	
	struct dataset* data = malloc(sizeof(struct dataset) * (int) (measurement_duration / measurement_delta));
	double clk;
	
	// TODO: Actually make lateral translations
	// TODO: Figure out how the gyro measures orientation
		
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
	double term_vel = TERM_VEL;
	
	// RNG
	time_t t;
	srand((unsigned) time(&t));
	int direction = rand() % 360; // Direction of lateral translation, degrees from east
	int wind_change = 0;
	
	// Initial step, no motion
	data[i].temperature = GROUND_TEMP; // No change to temp
	data[i].rot = malloc(sizeof(double) * 3);
	data[i].gyro = malloc(sizeof(double*) * 3);
	for(int j = 0; j < 3; j++){
		data[i].gyro[j] = malloc(sizeof(double) * 3);
		data[i].rot[j] = 0;
		for(int k = 0; k < 3; k++)
			data[i].gyro[j][k] = 0;		// Fixed starting orientation
	}
	data[i].gyro[0][0] = 1.0;
	data[i].gyro[1][1] = 1.0;
	data[i].gyro[2][2] = 1.0;
	set_axis(&(data[i].x), 0, 0, 0);	// No motion
	set_axis(&(data[i].y), 0, 0, 0);
	set_axis(&(data[i].z), 0, 0, 0);
	
	clk += measurement_delta;
	i++;
	
	while(clk < measurement_duration){
	//	printf("Clock step: %f\n", clk);
		char buffer[256];
		memset(buffer, '\0', sizeof(buffer));
		
		// Increment stage count if it's passed the timer
		if(launch_profile[stage_count][0] > 0 && clk > launch_profile[stage_count][0]){
			printf("launch_profile[%d] has expiry time %f, time is %f. Incrementing\n", stage_count, launch_profile[stage_count][0], clk);
			stage_count++;
		}
		
		// Rotation
		data[i].rot = malloc(sizeof(double) * 3);
		for(int j = 0; j < 3; j++)
			data[i].rot[j] = data[i-1].rot[j];
		
	//	printf("Torque\n");
		if(data[i-1].y.dista != 0.0){
			data[i].rot[GYRO_Y] += data[i-1].y.veloc / data[i-1].y.dista; // Moving faster at lower altitudes causes the most rotation
			
		}else{
			data[i].rot[GYRO_X] = 0;
			data[i].rot[GYRO_Y] = 0; // No spinning once we've landed
			data[i].rot[GYRO_Z] = 0;
		}
	//	torque[GYRO_X] = MIN(sin(direction * PI / 180) / data[i-1].y.veloc, 1);
	//	torque[GYRO_Z] = MIN(cos(direction * PI / 180) / data[i-1].y.veloc, 1);
	//	printf("Orientation\n");
		data[i].gyro = malloc(sizeof(double*) * 3);
		for(int j = 0; j < 3; j++){
			data[i].gyro[j] = malloc(sizeof(double) * 3);
			for(int k = 0; k < 3; k++)
				data[i].gyro[j][k] = data[i-1].gyro[j][k];
		}
		update_alignment(data[i].gyro, data[i].rot);
		
		
		// Update vertical acceleration
	//	printf("Vertical acceleration\n");
		data[i].y.accel = launch_profile[stage_count][1];
		if(launch_profile[stage_count][2] == 1.0 && data[i-1].y.dista != 0.0)
			data[i].y.accel += (data[i-1].y.veloc < 0 ? 1 : -1) * (pow(data[i-1].y.veloc, 2) * 32.17405) / (data[i-1].y.dista * pow(term_vel, 2.0));
			
		// Vertical motion
		data[i].y.veloc = data[i-1].y.veloc + measurement_delta * data[i].y.accel;
		data[i].y.dista = data[i-1].y.dista + measurement_delta * data[i].y.veloc;
		
		data[i].temperature = get_temperature(data[i].y.dista, GROUND_TEMP);
		
		// Lateral translation
		// X axis
		data[i].x.accel = sin(direction * PI / 180);
		data[i].x.veloc = data[i-1].x.veloc + measurement_delta * data[i].x.accel;
		data[i].x.dista = data[i-1].x.dista + measurement_delta * data[i].x.veloc;
		
		// Z axis
		data[i].z.accel = cos(direction * PI / 180);
		data[i].z.veloc = data[i-1].z.veloc + measurement_delta * data[i].z.accel;
		data[i].z.dista = data[i-1].z.dista + measurement_delta * data[i].z.veloc;
		
		// Can't fall through the earth
		if(data[i].y.dista <= 0.0){
			set_axis(&(data[i].y), 0, 0, 0);
			data[i].x.accel = 0;
			data[i].x.veloc = 0;
			data[i].z.accel = 0;
			data[i].z.veloc = 0;
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
		
		// Update wind every ~1000ft
		if(data[i].y.dista > wind_change + 1000 || data[i].y.dista < wind_change - 1000){
			direction = rand() % 360; // Direction of lateral translation, degrees from east(?)
			wind_change = data[i].y.dista;
		}	
	}
	
	return data;
}