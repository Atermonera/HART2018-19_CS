#include "datagen.h"

int main(){
	
	struct dataset* data;
	int launch_profile[5][3];
	
	// Launch profile configuration
	launch_profile[0][0] = 5;						// 5 seconds
	launch_profile[0][1] = 0;						// 0 acceleration
	launch_profile[0][2] = 0;						// No drag
	
	launch_profile[1][0] = 11;						// 6 seconds
	launch_profile[1][1] = BOOSTER_ACCELERATION;	// ~11.2G's acceleration
	launch_profile[1][2] = 0;						// No drag
	
	launch_profile[2][0] = 16;						// 5 seconds
	launch_profile[2][1] = GRAVITATION;				// Standard constant of gravition in ft/s^2
	launch_profile[2][2] = 0;						// No drag

	launch_profile[3][0] = 21;						// 5 seconds
	launch_profile[3][1] = SUSTAINER_ACCELERATION;	// ~22G's acceleration
	launch_profile[3][2] = 0;						// No drag
	
	launch_profile[4][0] = -1;						// Max duration
	launch_profile[4][1] = GRAVITATION;				// Standard constant of gravition in ft/s^2
	launch_profile[4][2] = 0;						// No drag
	
	double measurement_delta = 0.01; // Arbitrary: 100 measurement cycles per second
	double clk;
	int measurement_duration = 600; // 10m flight period in seconds
	
	data = gen_data(launch_profile, measurement_delta, measurement_duration);
	
	
	// Open the output file
	char outfile[50]; // File path
	memset(outfile, '\0', sizeof(outfile));
	time_t rawtime = time(NULL);
	struct tm *timeinfo = localtime(&rawtime);
	strftime(outfile, sizeof(outfile), "%F-%T.JSON", timeinfo);
	int file = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	for(int i = 0; i < (int) measurement_duration / measurement_delta; i++){
		char buffer[256];
		memset(buffer, '\0', sizeof(buffer));
		sprintf("{clock: %f, x_axis: {dist: %f, vel: %f, accel: %f}, y_axis: {dist: %f, vel: %f, accel: %f}, z_axis: {dist: %f, vel: %f, accel: %f}, temperature: %f, gyro: {x: {i: %f,j: %f, k: %f}, y: {i: %f,j: %f, k: %f}, z: {i: %f,j: %f, k: %f}}}\n", data[i].clk, data[i].x.dista, data[i].x.veloc, data[i].x.accel, data[i].y.dista, data[i].y.veloc, data[i].y.accel, data[i].z.dista, data[i].z.veloc, data[i].z.accel, data[i].temperature, data[i].gyro[0][0], data[i].gyro[0][1], data[i].gyro[0][2], data[i].gyro[1][0], data[i].gyro[1][1], data[i].gyro[1][2], data[i].gyro[2][0], data[i].gyro[2][1], data[i].gyro[2][2]);
		write(file, buffer, strlen(buffer));
	}
	
	close(file);
	
	return 0;
}