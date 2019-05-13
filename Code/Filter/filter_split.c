#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

// Variable array indexing
#define AXIS_X_DIST 	0
#define AXIS_X_VEL 		1
#define AXIS_X_ACCEL	2
#define AXIS_Y_DIST 	3
#define AXIS_Y_VEL 		4
#define AXIS_Y_ACCEL 	5
#define AXIS_Z_DIST 	6
#define AXIS_Z_VEL 		7
#define AXIS_Z_ACCEL	8

#define ROTATION_YAW	0
#define AXIS_YAW		1
#define ROTATION_PITCH	2
#define AXIS_PITCH		3
#define ROTATION_ROLL	4
#define AXIS_ROLL		5

#define TEMPERATURE 0

// Debug switches
#define VERBOSE 1
#define DEBUG 1
#define CRASH 1

// Predicted state
struct state{
	int size;				// Number of variables
	double** variables;		// Contains all measured variables
	double** observations;	// Observations
	double** p_k;			// Prediction error
	double** gain;			// Gain
	double** const_a;		// Timestep
	double** const_at;		//
	double** const_c;		// Observation-Truth ratio
	double** const_ct;		//
	double** const_q;		// Artificial noise
	double** const_r;		// Observation variance
};

double** matrix_transpose(double** a, int n, int m){
	double** b = malloc(sizeof(double*) * m);
	for(int i = 0; i < m; i++){
		b[i] = malloc(sizeof(double) * n);
		for(int j = 0; j < n; j++)
			b[i][j] = a[j][i];
	}
	
	return b;
}

void init_state(struct state* S, int size){
	S->size = size;
	S->variables = malloc(sizeof(double*) * S->size);
	S->observations = malloc(sizeof(double*) * S->size);
	S->p_k = malloc(sizeof(double*) * S->size);
	S->gain = malloc(sizeof(double*) * S->size);
	S->const_a = malloc(sizeof(double*) * S->size);
	S->const_c = malloc(sizeof(double*) * S->size);
	S->const_q = malloc(sizeof(double*) * S->size);
	S->const_r = malloc(sizeof(double*) * S->size);
	
	for(int i = 0; i < S->size; i++){
		S->p_k[i] = malloc(sizeof(double) * S->size);
		S->gain[i] = malloc(sizeof(double) * S->size);
		S->variables[i] = malloc(sizeof(double) * 1);
		S->observations[i] = malloc(sizeof(double) * 1);
		S->const_a[i] = malloc(sizeof(double) * S->size);
		S->const_c[i] = malloc(sizeof(double) * S->size);
		S->const_q[i] = malloc(sizeof(double) * S->size);
		S->const_r[i] = malloc(sizeof(double) * S->size);
		
		// Initialize matrices to empty matrices
		S->variables[i][0] = 0;
		S->observations[i][0] = 0;
		for(int j = 0; j < S->size; j++){
			S->p_k[i][j] = 0;
			S->gain[i][j] = 0;
			S->const_a[i][j] = 0;
			S->const_c[i][j] = 0;
			S->const_q[i][j] = 0;
			S->const_r[i][j] = 0;
		}
	}
	// Matrix transposes
	S->const_at = matrix_transpose(S->const_a, S->size, S->size);
	S->const_ct = matrix_transpose(S->const_c, S->size, S->size);
	return;
}

void free_state(struct state* S){
	for(int i = 0; i < 22; i++){
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
//printf("multiplying %d x %d by %d x %d\n", n, m, m, p);
	for(int i = 0; i < n; i++){
		for(int j = 0; j < p; j++){
//printf("clearing result[%d][%d]\n", i, j);
			result[i][j] = 0;
			for(int k = 0; k < m; k++){
//printf("k = %d\t", k);
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
	int i = 1;
	char* nextchar = malloc(sizeof(char)*2);
	memset(nextchar, '\0', 2);
	for(; strcmp(nextchar, "\n") && i != 0; i = read(fd, nextchar, 1)){
		if(!i)
			return 0;
		strcat(lineptr, nextchar);
	}
//	printf("Read line %s\n", lineptr);
	free(nextchar);
	return strlen(lineptr);
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
	struct state* curr - state struct containing the predicted state of the measured variables at time t-1, observations at time t
	double timestep - time in seconds between steps
	int verbose - debug logging
	int debug - debug logging
	int crash - debug logging
Returns: curr is updated, no return
***************/
void kalman_filter_step(struct state* curr, double timestep, int verbose, int debug, int crash){
	int i, j;
//printf("size = %d\n", curr->size);
	// Temp variables
	double** temp_var = malloc(sizeof(double*) * curr->size);
	double** temp_sq0 = malloc(sizeof(double*) * curr->size);
	double** temp_sq1 = malloc(sizeof(double*) * curr->size);
	for(i = 0; i < curr->size; i++){
		temp_var[i] = malloc(sizeof(double) * 1);
	//	temp_var[i][0] = 0;
		temp_sq0[i] = malloc(sizeof(double) * curr->size);
		temp_sq1[i] = malloc(sizeof(double) * curr->size);
	}
		
	if(debug){
		printf("\n******************\n");
		printf("DEBUG: CONSTANT MATRICES\n\n");
		printf("A:\n");
		matrix_print(curr->const_a, curr->size, curr->size);
		printf("\nAt:\n");
		matrix_print(curr->const_at, curr->size, curr->size);
		printf("\nC:\n");
		matrix_print(curr->const_c, curr->size, curr->size);
		printf("\nCt:\n");
		matrix_print(curr->const_ct, curr->size, curr->size);
		printf("\n******************\n");
	}
	if(verbose){
		printf("\nINFO: VARIABLE MATRICES\n\n");
		printf("Variables:\n");
		matrix_print(curr->variables, curr->size, 1);
		printf("\nINFO: Observation\n");
		matrix_print(curr->observations, curr->size, 1);
		printf("\nGain:\n");
		matrix_print(curr->gain, curr->size, curr->size);
		printf("\np_k:\n");
		matrix_print(curr->p_k, curr->size, curr->size);
		printf("\n*******************\n");
	} 	
	
//printf("temp_var: %d\n", (int) temp_var);
//printf("temp_var[0]: %d\n", (int) temp_var[0]);
//matrix_print(temp_var, curr->size, 1);
	
	// Prediction step:
	matrix_mult(temp_var, curr->const_a, curr->variables, curr->size, curr->size, 1, 0);
		
	// Prediction error
	// observation.p_k is a temp array
	// P_k = A * P_(k-1) * At + Q
	matrix_mult(temp_sq0, curr->const_a, curr->p_k, curr->size, curr->size, curr->size, 0);
	matrix_mult(curr->p_k, temp_sq0, curr->const_at, curr->size, curr->size, curr->size, 0);
	
	for(int i = 0; i < curr->size; i++){
		curr->variables[i][0] = temp_var[i][0]; // Have to move the predicted variables back into curr->variables
		for(int j = 0; j < curr->size; j++)
			curr->p_k[i][j] += curr->const_q[i][j]; // Last step of P_k prediction calculation, since we're iterating over the width
	}
	if(verbose){
		printf("\n******************\n");
		printf("INFO: PREDICTION STEP\n\n");
		printf("Variables:\n");
		matrix_print(curr->variables, curr->size, 1);
		printf("\np_k:\n");
		matrix_print(curr->p_k, curr->size, curr->size);
		printf("\n******************\n");
	}
	
	
	if(debug)
		printf("\nDEBUG: UPDATE STEP\n\n");
		
	// Update step:
	// x_hat gets x_hat + gain*(z-C*x_hat)
	// C * x_hat
	matrix_mult(temp_var, curr->const_c, curr->variables, curr->size, curr->size, 1, 0);
	
	if(debug){
		printf("\nCONST_C * X_hat:\n");
		matrix_print(temp_var, 22, 1);
	}
	
	// z - (C*x_hat)
	for(int i = 0; i < curr->size; i++){
		curr->observations[i][0] -= temp_var[i][0];
	}
	
	if(debug){
		printf("\nZ - (CONST_C * X_hat):\n");
		matrix_print(curr->observations, curr->size, 1);
	}	
	
	// gain*(z-C*x_hat)
	matrix_mult(temp_var, curr->gain, curr->observations, curr->size, curr->size, 1, 0);
	
	if(debug){
		printf("\ngain * (Z - (CONST_C * X_hat)):\n");
		matrix_print(temp_var, curr->size, 1);
	}	
	
	// x_hat <= x_hat + gain*(z-C*x_hat)
	for(i = 0; i < curr->size; i++)
		curr->variables[i][0] += temp_var[i][0];
	
	if(debug){
		printf("\nX_hat <= X_hat + gain * (Z - (CONST_C * X_hat)):\n");
		matrix_print(curr->variables, curr->size, 1);
	}	
	
	
	// G_k=P_k*C_transpose*(C*P_k*C_(transpose)+R)^(−1)
	// holds inverse P_k
	for(i = 0; i < curr->size; i++)
		for(j = 0; j< curr->size; j++)
			temp_sq0[i][j] = curr->p_k[i][j];
	
	if(debug){
		printf("\np_k + R:\n");
		matrix_print(temp_sq0, curr->size, curr->size);
	}		
	
	// C*P_k*Ct
	matrix_mult(temp_sq1, curr->const_c, temp_sq0, curr->size, curr->size, curr->size, 0);
	matrix_mult(temp_sq0, temp_sq1, curr->const_ct, curr->size, curr->size, curr->size, 0);

	// (C*P-k*Ct + R)
	for(i = 0; i < curr->size; i++)
		for(j = 0; j< curr->size; j++)
			temp_sq0[i][j] += curr->const_r[i][j];
	
	if(debug){
		printf("\nCONST_C * p_k * CONST_Ct + R:\n");
		matrix_print(temp_sq0, curr->size, curr->size);
	}	
		
	// Inversion, temp_sq0 inverts into temp_sq1
	inverse(temp_sq0, temp_sq1, curr->size);

	if(debug){
		printf("\ninverse(CONST_C * p_k * CONST_Ct + R):\n");
		matrix_print(temp_sq1, curr->size, curr->size);
	}	

	// P_k * Ct
	// gain = P_k*Ct*(C*P_k*Ct+R)^(-1)
	matrix_mult(temp_sq0, curr->p_k, curr->const_ct, curr->size, curr->size, curr->size, 0);
	matrix_mult(curr->gain, temp_sq0, temp_sq1, curr->size, curr->size, curr->size, 0);
	
	if(debug){
		printf("\ngain = P_k*Ct*inverse(C*P_k*Ct+R):\n");
		matrix_print(curr->gain, curr->size, curr->size);
	}		

	
	// P_k <- (I - gain * C) * P_k
	// gain * C
	matrix_mult(temp_sq0, curr->gain, curr->const_c, curr->size, curr->size, curr->size, 0);
	
	// I - (gain * C)
	// Copy P_k to temp
	for(i = 0; i < curr->size; i++){
		for(j = 0; j <curr->size; j++){
			temp_sq0[i][j] = (i == j ? 1 : 0) - temp_sq0[i][j];
			temp_sq1[i][j] = curr->p_k[i][j];
		}
	}
	matrix_mult(curr->p_k, temp_sq0, temp_sq1, curr->size, curr->size, curr->size, 0);
	
	if(debug){
		printf("\nP_k <- (I - gain * C) * P_k:\n");
		matrix_print(curr->p_k, curr->size, curr->size);
	}

	
	// Cleanup
	for(i = 0; i < curr->size; i++){
		free(temp_var[i]);
		free(temp_sq0[i]);
		free(temp_sq1[i]);
	}
	free(temp_var);
	free(temp_sq0);
	free(temp_sq1);
	
	if(crash)
		exit(1);
	
	return;
}



int main(){
	struct state* kinematics = malloc(sizeof(struct state));
	struct state* rotation = malloc(sizeof(struct state));
	struct state* temperature = malloc(sizeof(struct state));
	double* observation = malloc(sizeof(double) * 14);	// Holds variables read from file that we don't directly measure (and therefore toss)
	double clk;
	double timestep = 0.01; // 0.01s between observations
	init_state(kinematics, 9);
	init_state(rotation, 6);
	init_state(temperature, 1);
	
	
	// Configure matrices
	
	// kinematics
	// constant values
	for(int i = 0; i < kinematics->size; i++){
		kinematics->const_a[i][i] = 1;
		kinematics->const_c[i][i] = 1;
		kinematics->const_q[i][i] = 0.01;
		kinematics->const_r[i][i] = 0.4;
		kinematics->p_k[i][i] = 1.0;
		kinematics->gain[i][i] = 1.0;
	}		
	// X axis
	kinematics->const_a[AXIS_X_VEL][AXIS_X_DIST] 		= timestep;
	kinematics->const_a[AXIS_X_ACCEL][AXIS_X_VEL] 		= timestep;
	// Y axis
	kinematics->const_a[AXIS_Y_VEL][AXIS_Y_DIST] 		= timestep;
	kinematics->const_a[AXIS_Y_ACCEL][AXIS_Y_VEL] 		= timestep;
	// Z axis
	kinematics->const_a[AXIS_Z_VEL][AXIS_Z_DIST] 		= timestep;
	kinematics->const_a[AXIS_Z_ACCEL][AXIS_Z_VEL] 		= timestep;
	
	// rotation
	for(int i = 0; i < rotation->size; i++){
		rotation->const_a[i][i] = 1;
		rotation->const_c[i][i] = 1;
		rotation->const_q[i][i] = 0.01;
		rotation->const_r[i][i] = 0.1;
		rotation->p_k[i][i] = 1.0;
		rotation->gain[i][i] = 1.0;
	}	
	rotation->const_a[ROTATION_YAW][AXIS_YAW] 			= timestep;	
	rotation->const_a[ROTATION_PITCH][AXIS_PITCH]		= timestep;
	rotation->const_a[ROTATION_ROLL][AXIS_ROLL] 			= timestep;
	
	// temperature
	temperature->const_a[TEMPERATURE][TEMPERATURE] = 1;
	temperature->const_c[TEMPERATURE][TEMPERATURE] = 1;
	temperature->const_q[TEMPERATURE][TEMPERATURE] = 0.01;
	temperature->const_r[TEMPERATURE][TEMPERATURE] = 0.4;	
	temperature->p_k[TEMPERATURE][TEMPERATURE] = 1.0;
	temperature->gain[TEMPERATURE][TEMPERATURE] = 1.0;
	
	printf("Opening files\n");
	// Get data
	char infile[64] = "sst_noisy.csv";
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
	int count = 0;
	
	printf("Files open\n");
	
	// Iterate data through filter
	// readLine returns 0 on EOF
	while(readLine(line, fp)){
	//	printf("Iteration %d\t read %d: %s\n", count, strlen(line), line);
		sscanf(line, "%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf\n", &(clk), &(observation[0]), &(observation[1]), &(kinematics->observations[AXIS_X_ACCEL][0]), &(kinematics->observations[AXIS_Y_DIST][0]), &(observation[2]), &(kinematics->observations[AXIS_Y_ACCEL][0]), &(observation[3]), &(observation[4]), &(kinematics->observations[AXIS_Z_ACCEL][0]), &(temperature->observations[TEMPERATURE][0]), &(observation[5]), &(observation[6]), &(observation[7]), &(observation[8]), &(observation[9]), &(observation[10]), &(observation[11]), &(observation[12]), &(observation[13]), &(rotation->observations[ROTATION_ROLL][0]), &(rotation->observations[ROTATION_YAW][0]), &(rotation->observations[ROTATION_PITCH][0]));
		memset(line, '\0', sizeof(line));
		
	//	printf("Line parsed\n");
				
		printf("Analyzing step %d\n", count++);
		// Kinematics step
		kalman_filter_step(kinematics, 0.01, 0, 0, 0);
	//	kalman_filter_step(kinematics, 0.01, count > -1 ? VERBOSE : 0, count > -1 ? DEBUG : 0, count > 2 ? CRASH : 0);
		
		// Rotation step
		kalman_filter_step(rotation, 0.01, 0, 0, 0);
	//	kalman_filter_step(rotation, 0.01, count > -1 ? VERBOSE : 0, count > -1 ? DEBUG : 0, count > 2 ? CRASH : 0);
		
		// Temperature step
		kalman_filter_step(temperature, 0.01, 0, 0, 0);
	//	kalman_filter_step(temperature, 0.01, count > -1 ? VERBOSE : 0, count > -1 ? DEBUG : 0, count > 2 ? CRASH : 0);
		
		// Sanity check
		
		
		
	//	printf("\n************************************************\np_k:\n");
	//	matrix_print(curr->p_k, 3, 3);
	//	printf("\n************************************************\n");

		
		// Write new data to file
		char buffer[1024];
		memset(buffer, '\0', sizeof(buffer));
		sprintf(buffer, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n", clk, kinematics->variables[AXIS_X_DIST][0], kinematics->variables[AXIS_X_VEL][0], kinematics->variables[AXIS_X_ACCEL][0], kinematics->variables[AXIS_Y_DIST][0], kinematics->variables[AXIS_Y_VEL][0], kinematics->variables[AXIS_Y_ACCEL][0], kinematics->variables[AXIS_Z_DIST][0], kinematics->variables[AXIS_Z_VEL][0], kinematics->variables[AXIS_Z_ACCEL][0], temperature->variables[TEMPERATURE][0], rotation->variables[AXIS_YAW][0], rotation->variables[ROTATION_YAW][0], rotation->variables[AXIS_PITCH][0], rotation->variables[ROTATION_PITCH][0], rotation->variables[AXIS_ROLL][0], rotation->variables[ROTATION_ROLL][0]);
		write(output, buffer, strlen(buffer));
	}
	
	free_state(kinematics);
	free_state(rotation);
	free_state(temperature);
	// Close files
	close(fp);
	close(output);
	return 0;

}