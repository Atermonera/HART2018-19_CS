#include "datagen.h"

#include <errno.h>

#define PI 3.14159265

// Variable array indexing
// #define AXIS_X_DIST 
// #define AXIS_X_VEL 
// #define AXIS_X_ACCEL 

#define AXIS_Y_DIST 0
#define AXIS_Y_VEL 1
#define AXIS_Y_ACCEL 2

// #define AXIS_Z_DIST 
// #define AXIS_Z_VEL 
// #define AXIS_Z_ACCEL 
// #define TEMPERATURE
// #define ROTATION_X
// #define ROTATION_Y
// #define ROTATION_Z
// #define GYRO_X_i
// #define GYRO_X_j
// #define GYRO_X_k
// #define GYRO_Y_i
// #define GYRO_Y_j
// #define GYRO_Y_k
// #define GYRO_Z_i
// #define GYRO_Z_j
// #define GYRO_Z_k

// Predicted state
struct state{
	double clk;
	double** variables;		// Contains all measured variables
	double** p_k;			// Prediction error
	double** gain;			// Gain
};

// Set of observations for timestep k
struct observe{
//	double gyro[3];		// Rotation about gyro axes
//	double accel[3];	// Acceleration along gyro axes
	double baro;		// Barometric altitude
//	double temp;		// Temperature
};

void init_state(struct state* S){
	S->variables = malloc(sizeof(double*) * 3);
	S->p_k = malloc(sizeof(double*) * 3);
	S->gain = malloc(sizeof(double*) * 3);
	
	for(int i = 0; i < 3; i++){
		S->p_k[i] = malloc(sizeof(double) * 3);
		S->gain[i] = malloc(sizeof(double) * 3);
		S->variables[i] = malloc(sizeof(double) * 1);
	}
	
	return;
}

void free_state(struct state* S){
	for(int i = 0; i < 3; i++){
		free(S->p_k[i]);
		free(S->gain[i]);
	}
	
	free(S->variables);
	free(S->p_k);
	free(S->gain);
	
	return;
}

// Multiples matrix A(n x m) by matrix B (m x p), stores result in matrix result. Result must already be allocated in the correct size
void matrix_mult(double** result, double** A, double** B, int n, int m, int p, int verbose){
	for(int i = 0; i < n; i++){
		for(int j = 0; j < p; j++){
			result[i][j] = 0;
			for(int k = 0; k < m; k++){
				if(verbose)
					printf("result[%d][%d] += A[%d][%d] * B[%d][%d]\n %lf += %lf * %lf\n", i, j, k, i, j, k, result[i][j], A[k][i], B[j][k]);
				result[i][j] += A[i][k] * B[k][j];
			}
			if(verbose)
				printf("multiplication: result[%d][%d] = %lf\n", i, j, result[i][j]);
		}
	}
	return;
}

void matrix_print(double** a, int n, int m){
	for(int i = 0; i < n; i++)
		printf("\t%d\t", i);
	printf("\n");
	for(int i = 0; i < m; i++){
		printf("%d", i);
		for(int j = 0; j < n; j++)
			printf("\t%lf", a[j][i]);
		printf("\n");
	}
	
	return;
}

// Reads in the next line from fd into lineptr. Returns 0 on success
// Like getLine but it works on file descriptors and doesn't keep the \n
int readLine(char* lineptr, int fd){
	int i;
	char* nextchar = malloc(sizeof(char)*2);
	memset(nextchar, '\0', 2);
	for(; strcmp(nextchar, "\n"); i = read(fd, nextchar, 1))
		strcat(lineptr, nextchar);
//	printf("Read line %s\n", lineptr);
	free(nextchar);
	return strlen(lineptr);
}

double** matrix_transpose(double** a, int n, int m){
	double** b = malloc(sizeof(double*) * m);
	for(int i = 0; i < m; i++){
		b[i] = malloc(sizeof(double) * n);
		for(int j = 0; j < n; j++)
			b[i][j] = a[j][i];
	}
	
	return b;
}

// Constant matrix building
double** const_A(double timestep){
	double** m = malloc(sizeof(double*) * 3);
	for(int i = 0; i < 3; i++){
		m[i] = malloc(sizeof(double) * 3);
		for(int j = 0; j < 3; j++)
			m[i][j] = 0;
	}
	
	// Manual index assignment for non-zero values
	m[0][0] = 1;
	m[1][0] = timestep;
	m[1][1] = 1;
	m[2][1] = timestep;
	m[2][2] = 1;
	
	return m;
}

double** const_C(){
	double** m = malloc(sizeof(double*) * 3);
	for(int i = 0; i < 3; i++){
		m[i] = malloc(sizeof(double) * 3);
		for(int j = 0; j < 3; j++)
			m[i][j] = 0;
	}
	
	// Manual index assignment for non-zero values
	m[0][0] = 1;
	m[1][1] = 1;
	m[2][2] = 1;
	
	return m;
}

double** const_Q(){
	double** m = malloc(sizeof(double*) * 3);
	for(int i = 0; i < 3; i++){
		m[i] = malloc(sizeof(double) * 3);
		for(int j = 0; j < 3; j++)
			m[i][j] = (i == j ? 1 : 0);
	}
	
	// Manual index assignment for non-zero values
	m[0][0] = 0;
	m[1][1] = 0;
	m[2][2] = 0;
	return m;
}

double** const_R(){
	double** m = malloc(sizeof(double*) * 3);
	for(int i = 0; i < 3; i++){
		m[i] = malloc(sizeof(double) * 3);
		for(int j = 0; j < 3; j++)
			m[i][j] = (i == j ? 1 : 0);
	}
	
	// Manual index assignment for non-zero values
	m[0][0] = 0.4;
	m[1][1] = 0.4;
	m[2][2] = 0.4;
	
	return m;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Adapted from:
// https://www.geeksforgeeks.org/adjoint-inverse-matrix/


// Function to get cofactor of A[p][q] in temp[][]. n is current 
// dimension of A[][] 
void getCofactor(double** A, double** temp, int p, int q, int n){ 
	int i = 0, j = 0; 
	
	// Looping for each element of the matrix 
	for (int row = 0; row < n; row++){ 
		for (int col = 0; col < n; col++){ 
			//  Copying into temporary matrix only those element 
			//  which are not in given row and column 
			if (row != p && col != q) { 
				temp[i][j++] = A[row][col]; 
				
				// Row is filled, so increase row index and 
				// reset col index 
				if (j == n - 1){ 
					j = 0; 
					i++; 
				} 
			} 
		} 
	}
	return;
} 
  
/* Recursive function for finding determinant of matrix. 
   n is current dimension of A[][]. */
double determinant(double** A, int N){ 
	double D = 0; // Initialize result 
	
	//  Base case : if matrix contains single element 
	if (N == 1) 
		return A[0][0]; 
	
	// temp is used to store cofactors of A[][] 
	int sign = 1;
	double** temp = malloc(sizeof(double*) * N);
	for(int i = 0; i < N; i++)
		temp[i] = malloc(sizeof(double) * N);
	
	// Iterate for each element of first row 
	for (int f = 0; f < N; f++){ 
		// Getting Cofactor of A[0][f] 
		getCofactor(A, temp, 0, f, N); 
		D += sign * A[0][f] * determinant(temp, N - 1); 
		
		// terms are to be added with alternate sign 
		sign = -sign; 
	} 
	
	for(int i = 0; i < N; i++)
		free(temp[i]);
	free(temp);
	
	return D; 
} 
  
// Function to get adjoint of A[N][N] in adj[N][N]. 
void adjoint(double** A, double** adj, int N){
	if (N == 1){ 
		adj[0][0] = 1; 
		return; 
	}
	
	// temp is used to store cofactors of A[][] 
	int sign = 1;
	double** temp = malloc(sizeof(double*) * N);
	for(int i = 0; i < N; i++)
		temp[i] = malloc(sizeof(double) * N);
	
	for (int i = 0; i < N; i++){ 
        for (int j = 0; j < N; j++){ 
			// Get cofactor of A[i][j] 
			getCofactor(A, temp, i, j, N); 
			
			// sign of adj[j][i] positive if sum of row 
			// and column indexes is even. 
			sign = ((i + j) % 2 == 0) ? 1 : -1; 
			
			// Interchanging rows and columns to get the 
			// transpose of the cofactor matrix 
			adj[j][i] = (sign)*(determinant(temp, N - 1)); 
		} 
	}
	
	for(int i = 0; i < N; i++)
		free(temp[i]);
	free(temp);
	
	return;
} 
  
// Function to calculate and store inverse
void inverse(double** A, double** inverse, int N){ 
	// Find determinant of A[][] 
	double det = determinant(A, N); 
	if (det == 0) 
		return;
	
	// Find adjoint  
    adjoint(A, inverse, N); 
	
	// Find Inverse using formula "inverse(A) = adj(A)/det(A)" 
	for (int i = 0; i < N; i++) 
		for (int j = 0; j < N; j++) 
			inverse[i][j] /= det; 
	
	return; 
} 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/***************
Function: kalman_filter_step
Description: 
	Updates state struct curr with predicted change, factors in observations according to gain
	observe structs provide observations from t, t-1, and t-2 timesteps, to measure changes in observation
Parameters:
	struct state* curr - state struct containing the predicted state of the craft as of time t-1
	struct observe* t0, t1, t2 - Observations at step k=t, t-1, t-2
	double timestep - time in seconds between steps
Returns: curr is updated, no return
***************/
void kalman_filter_step(struct state* curr, struct observe t0, struct observe t1, struct observe t2, double timestep, int verbose, int debug, int crash){
	int i, j;
	struct state observation;
	init_state(&observation);
	observation.clk = curr->clk;
	observation.variables[AXIS_Y_DIST][0]  = t0.baro;
	observation.variables[AXIS_Y_VEL][0]   = (t0.baro - t1.baro) / timestep;
	observation.variables[AXIS_Y_ACCEL][0] = (t0.baro - 2 * t1.baro + t2.baro) / (timestep * timestep);
/*	
	if(verbose){
		printf("\n***********************\n");
		printf("DEBUG: Observation translation\n");
		printf("Altitude: %lf = %lf\n", observation.variables[AXIS_Y_DIST], t0.baro);
		printf("Velocity: %lf = (%lf - %lf) / %lf\n", observation.variables[AXIS_Y_VEL], t0.baro, t1.baro, timestep);
		printf("Accelera: %lf = (%lf - %lf) / (%lf * %lf)", observation.variables[AXIS_Y_ACCEL], t0.baro, t2.baro, timestep, timestep);
		printf("\n***********************\n");	}
 */	
//	observation.variables[AXIS_X_DIST]  = 0;
//	observation.variables[AXIS_X_VEL]   = 0;
//	observation.variables[AXIS_X_ACCEL] = 0;

//	observation.variables[AXIS_Z_DIST]  = 0;
//	observation.variables[AXIS_Z_VEL]   = 0;
//	observation.variables[AXIS_Z_ACCEL] = 0;
	
	for(int i = 0; i < 3; i++){
		for(int j = 0; j < 3; j++){
			observation.p_k[i][j] = 0;
			observation.gain[i][j] = 0;
		}
	}
	
	if(verbose){
		printf("\n***********************\n");
		printf("INFO: Observation\n");
		matrix_print(observation.variables, 3, 1);
		printf("\n***********************\n");
	}
	
//	double temperature;		// Temperature
//	double gyro[3][3];		// unit vectors for the axes of the accelerometer
//	double rot[3];			// Rate of rotation about each axis
//	double kinematics[3][3];// Motion along each axis
//	double p_k[3][3];		// Prediction error	
	double** CONST_A = const_A(timestep);
	double** CONST_At = matrix_transpose(CONST_A, 3, 3);
	double** CONST_C = const_C();
	double** CONST_Ct = matrix_transpose(CONST_C, 3, 3);
	double** CONST_R = const_R();
	double** CONST_Q = const_Q();
	double** temp = malloc(sizeof(double*) * 3);
	for(int i = 0; i < 3; i++)
		temp[i] = malloc(sizeof(double) * 1);
	
	if(debug){
		printf("\n******************\n");
		printf("DEBUG: CONSTANT MATRICES\n\n");
		printf("A:\n");
		matrix_print(CONST_A, 3, 3);
		printf("\nAt:\n");
		matrix_print(CONST_At, 3, 3);
		printf("\nC:\n");
		matrix_print(CONST_C, 3, 3);
		printf("\nCt:\n");
		matrix_print(CONST_Ct, 3, 3);
		printf("\n******************\n");
	}
	if(verbose){
		printf("\nINFO: VARIABLE MATRICES\n\n");
		printf("Variables:\n");
		matrix_print(curr->variables, 3, 1);
		printf("\nGain:\n");
		matrix_print(curr->gain, 3, 3);
		printf("\np_k:\n");
		matrix_print(curr->p_k, 3, 3);
		printf("\n*******************\n");
	} 
	
	// Prediction step:
	
	// Iterate through the axes
		
	// Kinematics
	// Assume constant acceleration for now
	/*
	*		 [d(t)]   [1 t 0]	[d(t-1)]
	*	x_ = [v(t)] = [0 1 t] * [v(t-1)]
	*	 k	 [a(t)]   [0 0 1]	[a(t-1)]
	*/
	curr->variables[AXIS_Y_DIST][0] = curr->variables[AXIS_Y_DIST][0] + timestep * curr->variables[AXIS_Y_VEL][0];
	curr->variables[AXIS_Y_VEL][0] = curr->variables[AXIS_Y_VEL][0] + timestep * curr->variables[AXIS_Y_ACCEL][0];
	
	// Prediction error
	// observation.p_k is a temp array
	// P_k = A * P_(k-1) * At + Q
	matrix_mult(observation.p_k, CONST_A, curr->p_k, 3, 3, 3, 0);
	matrix_mult(curr->p_k, observation.p_k, CONST_At, 3, 3, 3, 0);

	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			curr->p_k[i][j] += CONST_Q[i][j];
	
	if(verbose){
		printf("\n******************\n");
		printf("INFO: PREDICTION STEP\n\n");
		printf("Variables:\n");
		matrix_print(curr->variables, 3, 1);
		printf("\np_k:\n");
		matrix_print(curr->p_k, 3, 3);
		printf("\n******************\n");
	}
	if(debug)
		printf("\nDEBUG: UPDATE STEP\n\n");
	
	// Update step:
	// x_hat gets x_hat + gain*(z-C*x_hat)
	// C * x_hat
	matrix_mult(temp, CONST_C, curr->variables, 3, 3, 1, 0);
	
	if(debug){
		printf("\nCONST_C * X_hat:\n");
		matrix_print(temp, 3, 1);
	}
	
	// z - (C*x_hat)
	for(int i = 0; i < 3; i++){
		observation.variables[i][0] -= temp[i][0];
	}
	
	if(debug){
		printf("\nZ - (CONST_C * X_hat):\n");
		matrix_print(observation.variables, 3, 1);
	}	
	
	for(int i = 0; i < 3; i++)
		temp[i][0] = 0.0;
	
	// gain*(z-C*x_hat)
	matrix_mult(temp, curr->gain, observation.variables, 3, 3, 1, 0);
	
	if(debug){
		printf("\ngain * (Z - (CONST_C * X_hat)):\n");
		matrix_print(temp, 3, 1);
	}	
	
	// x_hat <= x_hat + gain*(z-C*x_hat)
	for(i = 0; i < 3; i++)
		curr->variables[i][0] += temp[i][0];
	
	if(debug){
		printf("\nX_hat <= X_hat + gain * (Z - (CONST_C * X_hat)):\n");
		matrix_print(curr->variables, 3, 1);
	}	
	
	// G_k=P_k*C_transpose*(C*P_k*C_(transpose)+R)^(−1)
	// holds inverse P_k
	for(i = 0; i < 3; i++)
		for(j = 0; j< 3; j++)
			observation.p_k[i][j] = curr->p_k[i][j];
	
	if(debug){
		printf("\np_k + R:\n");
		matrix_print(observation.p_k, 3, 3);
	}		
	
	// C*P_k*Ct
	matrix_mult(observation.gain, CONST_C, observation.p_k, 3, 3, 3, 0);
	matrix_mult(observation.p_k, observation.gain, CONST_Ct, 3, 3, 3, 0);

	// (C*P-k*Ct + R)
	for(i = 0; i < 3; i++)
		for(j = 0; j< 3; j++)
			observation.p_k[i][j] += CONST_R[i][j];
	
	if(debug){
		printf("\nCONST_C * p_k * CONST_Ct + R:\n");
		matrix_print(observation.p_k, 3, 3);
	}	
		
	// Inversion, gain is temp
	inverse(observation.p_k, observation.gain, 3);

	if(debug){
		printf("\ninverse(CONST_C * p_k * CONST_Ct + R):\n");
		matrix_print(observation.gain, 3, 3);
	}	

	
	// P_k
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			curr->gain[i][j] = curr->gain[i][j];
	
	// P_k * Ct
	// gain = P_k*Ct*(C*P_k*Ct+R)^(-1)
	matrix_mult(observation.p_k, curr->p_k, CONST_Ct, 3, 3, 3, 0);
	matrix_mult(curr->gain, observation.p_k, observation.gain, 3, 3, 3, 0);
	
	if(debug){
		printf("\ngain = P_k*Ct*inverse(C*P_k*Ct+R):\n");
		matrix_print(curr->gain, 3, 3);
	}		
	
	// P_k <- (I - gain * C) * P_k

	// gain * C
	matrix_mult(observation.gain, curr->gain, CONST_C, 3, 3, 3, 0);
	
	// I - (gain * C)
	// Copy P_k to temp
	for(i = 0; i < 3; i++){
		for(j = 0; j < 3; j++){
			observation.gain[i][j] = (i == j ? 1 : 0) - observation.gain[i][j];
			observation.p_k[i][j] = curr->p_k[i][j];
		}
	}
	matrix_mult(curr->p_k, observation.gain, observation.p_k, 3, 3, 3, 0);
	
	if(debug){
		printf("\nP_k <- (I - gain * C) * P_k:\n");
		matrix_print(curr->p_k, 3, 3);
	}
	
	// SANITY CHECK
	if(curr->variables[AXIS_Y_DIST] < 0)
		for(int i = 0; i < 3; i++)
			curr->variables[i] = 0; // No motion once we've landed
	
	
	
	for(int i = 0; i < 3; i++){
		free(CONST_A[i]);
		free(CONST_At[i]);
		free(CONST_C[i]);
		free(CONST_Ct[i]);
		free(CONST_Q[i]);
	}
	
	free(CONST_A);
	free(CONST_At);
	free(CONST_C);
	free(CONST_Ct);
	free(CONST_Q);
	free_state(&observation);
	
	if(crash)
		exit(1);
	
	return;
}

int main(){
	struct state* curr = malloc(sizeof(struct state));
	struct state* observation = malloc(sizeof(struct state));
	init_state(curr);
	init_state(observation);
	
	printf("Initializing struct\n");
	for(int i = 0; i < 3; i++){
		curr->variables[i][0] = 0.0;
		for(int j = 0; j < 3; j++){
			curr->gain[i][j] = 0.0;
			curr->p_k[i][j] = 0.0;
		}
	}
	
	// Initialize error and gain
	curr->gain[0][0] = 1;
	curr->gain[1][1] = 1;
	curr->gain[2][2] = 1;
	curr->p_k[0][0] = 1;
	curr->p_k[1][1] = 1;
	curr->p_k[2][2] = 1;
	
	struct observe t0, t1, t2;
	t0.baro = 0;
	t1.baro = 0;
	t2.baro = 0;
	
	// temp variables
	double a0, a1, a2, a3, a4, a5, a6, a7, a8, a9;
	double b0, b1, b2, b3, b4, b5, b6, b7, b8, b9;
	
	printf("Opening files\n");
	// Get data
	char infile[64] = "2019-03-10-20h-59m-45s.csv";
	char outfile[64] = "kalman_out.CSV";
	// Need FILE pointer for geltine. Personally prefer integer file pointers, so that's used for output file
	int fp = open(infile, O_RDONLY);
	int output = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	char* line = malloc(sizeof(char) * 1024);
	memset(line, '\0', sizeof(line));
	printf("Passing line 0 buffer\n");
	readLine(line, fp);
//	printf("Read line %s\n", line);
	memset(line, '\0', sizeof(line));
	int len = 0;
	int count = 0;
	
	printf("Files open\n");
	
	// Iterate data through filter
	do{
		len = readLine(line, fp);
	//	printf("Iteration %d\t read %d: %s\n", count, len, line);
		sscanf(line, "%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf\n", &(observation->clk), &a0, &a1, &a2, &(observation->variables[AXIS_Y_DIST][0]), &(observation->variables[AXIS_Y_VEL][1]), &(observation->variables[AXIS_Y_ACCEL][2]), &a3, &a4, &a5, &a6, &a7, &a8, &a9, &b0, &b1, &b2, &b3, &b4, &b5);
		memset(line, '\0', sizeof(line));
		
	//	printf("Line parsed\n");
		
		// Update the observations
		t2.baro = t1.baro;
		t1.baro = t0.baro;
		t0.baro = observation->variables[AXIS_Y_DIST][0];
		
		printf("Analyzing step %d\n", count++);
		kalman_filter_step(curr, t0, t1, t2, 0.01, 0, 0, 0);
	//	kalman_filter_step(curr, t0, t1, t2, 0.01, count > 52 ? 1 : 0, count > 55 ? 1 : 0, count > 57 ? 1 : 0);
		
		
	//	printf("\n************************************************\np_k:\n");
	//	matrix_print(curr->p_k, 3, 3);
	//	printf("\n************************************************\n");

		
		// Write new data to file
		char buffer[1024];
		memset(buffer, '\0', sizeof(buffer));
		sprintf(buffer, "%f, %f, %f, %f\n", curr->clk, curr->variables[AXIS_Y_DIST][0], curr->variables[AXIS_Y_VEL][0], curr->variables[AXIS_Y_ACCEL][0]);
		write(output, buffer, strlen(buffer));
	} while(len != 1);
	
	free_state(curr);
	free_state(observation);
	// Close files
	close(fp);
	close(output);
	return 0;
}