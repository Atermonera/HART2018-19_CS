#include "datagen.h"
#define CSV 0
#define JSON 1

// Box-Muller transform, for random normal distribution
double get_rand(){
	double u = (double) rand() / RAND_MAX;
	double v = (double) rand() / RAND_MAX;
	double x = sqrt(-2 * log(u)) * cos(2 * PI * v);
//	printf("Generated: sqrt(-2 * log(%f)) * cos(2 * PI * %f) = %f\n", u, v, x);
	return x;
}

int main(int argc, char* argv[]){
	// SUSTAINER SIMULATION
	struct dataset* data;
	double** launch_profile = malloc(sizeof(double*) * 5);
	for(int i = 0; i < 5; i++)
		launch_profile[i] = malloc(sizeof(double) * 3);
	
	// Launch profile configuration
	launch_profile[0][0] = 5;						// 5 seconds
	launch_profile[0][1] = 0;						// 0 acceleration
	launch_profile[0][2] = 0.0;						// No drag
	
	launch_profile[1][0] = 11;						// 6 seconds
	launch_profile[1][1] = BOOSTER_ACCELERATION;	// ~11.2G's acceleration
	launch_profile[1][2] = 1.0;						// No drag
	
	launch_profile[2][0] = 16;						// 5 seconds
	launch_profile[2][1] = GRAVITATION;				// Standard constant of gravition in ft/s^2
	launch_profile[2][2] = 1.0;						// No drag

	launch_profile[3][0] = 21;						// 5 seconds
	launch_profile[3][1] = SUSTAINER_ACCELERATION;	// ~22G's acceleration
	launch_profile[3][2] = 1.0;						// No drag
	
	launch_profile[4][0] = -1;						// Max duration
	launch_profile[4][1] = GRAVITATION;				// Standard constant of gravition in ft/s^2
	launch_profile[4][2] = 1.0;						// Drag
	
	double measurement_delta = 0.01; // Arbitrary: 100 measurement cycles per second
	int measurement_duration = 240; // 10m flight period in seconds
	int mode = -1;

	// RNG
	time_t t;
	srand((unsigned) time(&t));
	
	// Open the output file
	char outfile[50]; // File path
	memset(outfile, '\0', sizeof(outfile));
	printf("argc: %d\n", argc);
	if(argc < 3){
		printf("Default file naming\n");
		time_t rawtime = time(NULL);
		struct tm *timeinfo = localtime(&rawtime);
		strftime(outfile, sizeof(outfile), "sst_noisy_%F-%Hh-%Mm-%Ss", timeinfo);
	} else {
		if(strlen(argv[2]) <= 50)
			strcpy(outfile, argv[2]);
		else {
			printf("Error: Sustainer output file path too long.\n");
			exit(-3);
		}
	}
	if(argc < 2 || !strcmp(argv[1], "CSV")){
		strcat(outfile, ".csv");
		mode = CSV;
	}
	else if(!strcmp(argv[1], "JSON")){
		strcat(outfile, ".JSON");
		mode = JSON;
//	} else{
//		printf("Error: Invalid extension. Correct format:\nnoisy_data [file extension] [file name]\n");
//		exit(-2);
	}
	
	data = gen_data(launch_profile, measurement_delta, measurement_duration);
	
	printf("Data complete\n");	
	
	int file = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	printf("Output open: %s\n", outfile);
	
	char buffer[4096];
	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "clock, x_dist, x_vel, x_accel, y_dist, y_vel, y_accel, z_dist, z_vel, z_accel, temperature, gyro_yaw, gyro_pitch, gyro_roll, rot_yaw, rot_pitch, rot_roll, latitude, longitude\n");
	write(file, buffer, strlen(buffer));
	
	for(int i = 0; i <= (int) measurement_duration / measurement_delta; i++){
//		printf("PRINTING %f\n", data[i].clk);
		// Add noise
		data[i].temperature += 0.64 * get_rand();
		data[i].x.dista += data[i].x.dista * get_rand() / 10000;
		data[i].x.veloc += data[i].x.veloc * get_rand() / 10;
		data[i].x.accel += data[i].x.accel * get_rand() / 10;
		data[i].y.dista += data[i].y.dista * get_rand() / 10000;
		data[i].y.veloc += data[i].y.veloc * get_rand() / 1000;
		data[i].y.accel += data[i].y.accel * get_rand() / 100;
		data[i].z.dista += data[i].z.dista * get_rand() / 10000;
		data[i].z.veloc += data[i].z.veloc * get_rand() / 10;
		data[i].z.accel += data[i].z.accel * get_rand() / 10;

		// Gyro noise will be random, small rotations about each axis
		data[i].rot[0] += data[i].rot[0] * get_rand();
		data[i].rot[1] += data[i].rot[1] * get_rand();
		data[i].rot[2] += data[i].rot[2] * get_rand();
	
		memset(buffer, '\0', sizeof(buffer));
		if(mode == CSV) // CSV
			sprintf(buffer, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n", data[i].clk, data[i].x.dista, data[i].x.veloc, data[i].x.accel, data[i].y.dista, data[i].y.veloc, data[i].y.accel, data[i].z.dista, data[i].z.veloc, data[i].z.accel, data[i].temperature, data[i].gyro[0], data[i].gyro[1], data[i].gyro[2], data[i].rot[0], data[i].rot[1], data[i].rot[2], (32.99038889 + data[i].z.dista / 364320), (105.0417222 + data[i].x.dista / 364320));
		else if(mode == JSON)		// JSON
			sprintf(buffer, "{clock: %f, x_axis: {dist: %f, vel: %f, accel: %f}, y_axis: {dist: %f, vel: %f, accel: %f}, z_axis: {dist: %f, vel: %f, accel: %f}, temperature: %f, gyro: {yaw: %f, pitch: %f, roll: %f}, rot: {yaw: %f, pitch: %f, roll: %f)}\n", data[i].clk, data[i].x.dista, data[i].x.veloc, data[i].x.accel, data[i].y.dista, data[i].y.veloc, data[i].y.accel, data[i].z.dista, data[i].z.veloc, data[i].z.accel, data[i].temperature, data[i].gyro[0], data[i].gyro[1], data[i].gyro[2], data[i].rot[0], data[i].rot[1], data[i].rot[2]);
				
		write(file, buffer, strlen(buffer));
		free(data[i].gyro);
		free(data[i].rot);
	}
	
	for(int i = 0; i < 5; i++)
		free(launch_profile[i]);
	free(launch_profile);
	free(data);
	data = NULL;
	
	close(file);
	printf("Data written to: %s\n", outfile);

	
	// BOOSTER SIMULATION
	launch_profile = malloc(sizeof(double*) * 3);
	for(int i = 0; i < 3; i++)
		launch_profile[i] = malloc(sizeof(double) * 3);
	
	// Launch profile configuration
	launch_profile[0][0] = 5;						// 5 seconds
	launch_profile[0][1] = 0;						// 0 acceleration
	launch_profile[0][2] = 0.0;						// No drag
	
	launch_profile[1][0] = 11;						// 6 seconds
	launch_profile[1][1] = BOOSTER_ACCELERATION;	// ~11.2G's acceleration
	launch_profile[1][2] = 1.0;						// No drag
	
	launch_profile[2][0] = -1;						// Max duration
	launch_profile[2][1] = GRAVITATION;				// Standard constant of gravition in ft/s^2
	launch_profile[2][2] = 1.0;						// Drag
		
	data = gen_data(launch_profile, measurement_delta, measurement_duration);
	
	printf("Data complete\n");
	// Open the output file
	memset(outfile, '\0', sizeof(outfile));
	if(argc < 3){
		outfile[0] = 'b';	// sst_[...] -> bst_[...]
	} else {
		if(strlen(argv[3]) <= 50){
			strcpy(outfile, argv[3]);
			if(mode == JSON)
				strcat(outfile, ".json");
			else
				strcat(outfile, ".csv");	// Default is csv, we've already enforced that the specified extension is valid.
		} else {
			printf("Error: Booster output file path too long.\n");
			exit(-3);
		}
	}
	file = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	printf("Output open: %s\n", outfile);
	
	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "clock, x_dist, x_vel, x_accel, y_dist, y_vel, y_accel, z_dist, z_vel, z_accel, temperature, gyro_yaw, gyro_pitch, gyro_roll, rot_yaw, rot_pitch, rot_roll, latitude, longitude\n");
	write(file, buffer, strlen(buffer));
	
	for(int i = 0; i <= (int) measurement_duration / measurement_delta; i++){
	//	printf("PRINTING %f\n", data[i].clk);
		// Add noise
		data[i].temperature += 0.64 * get_rand();
		data[i].x.dista += data[i].x.dista * get_rand() / 10000;
		data[i].x.veloc += data[i].x.veloc * get_rand() / 10;
		data[i].x.accel += data[i].x.accel * get_rand() / 10;
		data[i].y.dista += data[i].y.dista * get_rand() / 10000;
		data[i].y.veloc += data[i].y.veloc * get_rand() / 1000;
		data[i].y.accel += data[i].y.accel * get_rand() / 100;
		data[i].z.dista += data[i].z.dista * get_rand() / 10000;
		data[i].z.veloc += data[i].z.veloc * get_rand() / 10;
		data[i].z.accel += data[i].z.accel * get_rand() / 10;

		// Gyro noise will be random, small rotations about each axis
		data[i].rot[0] += data[i].rot[0] * get_rand();
		data[i].rot[1] += data[i].rot[1] * get_rand();
		data[i].rot[2] += data[i].rot[2] * get_rand();
	
		memset(buffer, '\0', sizeof(buffer));
		if(mode == CSV) // CSV
			sprintf(buffer, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n", data[i].clk, data[i].x.dista, data[i].x.veloc, data[i].x.accel, data[i].y.dista, data[i].y.veloc, data[i].y.accel, data[i].z.dista, data[i].z.veloc, data[i].z.accel, data[i].temperature, data[i].gyro[0], data[i].gyro[1], data[i].gyro[2], data[i].rot[0], data[i].rot[1], data[i].rot[2], (32.99038889 + data[i].z.dista / 364320), (105.0417222 + data[i].x.dista / 364320));
		else if(mode == JSON)		// JSON
			sprintf(buffer, "{clock: %f, x_axis: {dist: %f, vel: %f, accel: %f}, y_axis: {dist: %f, vel: %f, accel: %f}, z_axis: {dist: %f, vel: %f, accel: %f}, temperature: %f, gyro: {yaw: %f, pitch: %f, roll: %f}, rot: {yaw: %f, pitch: %f, roll: %f)}\n", data[i].clk, data[i].x.dista, data[i].x.veloc, data[i].x.accel, data[i].y.dista, data[i].y.veloc, data[i].y.accel, data[i].z.dista, data[i].z.veloc, data[i].z.accel, data[i].temperature, data[i].gyro[0], data[i].gyro[1], data[i].gyro[2], data[i].rot[0], data[i].rot[1], data[i].rot[2]);
				
		write(file, buffer, strlen(buffer));
		free(data[i].gyro);
		free(data[i].rot);
	}
	
	for(int i = 0; i < 3; i++)
		free(launch_profile[i]);
	free(launch_profile);
	free(data);
	
	close(file);
	printf("Data written to: %s\n", outfile);	
	
	return 0;
}