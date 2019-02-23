#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>

#define PI 3.14159265

// Kinematics array indexing
#define DIST 0
#define VEL 1
#define ACCEL 2

// Axis array indexing
#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

// Constant matrices
#define CONST_A  {{1, timestep, 0}, {0, 1, timestep}, {0, 0, 1}}
#define CONST_At {{1, 0, 0}, {timestep, 1, 0}, {0, timestep, 1}}
#define CONST_C  {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}
#define CONST_Ct {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}


// Predicted state
struct state{
	double clk;
//	double temperature;		// Temperature
//	double gyro[3][3];		// unit vectors for the axes of the accelerometer
//	double rot[3];			// Rate of rotation about each axis
	double kinematics[3][3];// Motion along each axis
	double p_k[3][3];		// Prediction error
	double gain[3][3];		// Gain
};

// Set of observations for timestep k
struct observe{
//	double gyro[3];		// Rotation about gyro axes
//	double accel[3];	// Acceleration along gyro axes
	double baro;		// Barometric altitude
//	double temp;		// Temperature
};

// Multiples matrix A(n x m) by matrix B (m x p), stores result in matrix result. Result must already be allocated in the correct size
void matrix_mult(double** result, double** A, double** B, int n, int m, int p){
	for(int i = 0; i < n; i++){
		for(int j = 0; j < p; j++){
			result[i][j] = 0;
			for(int k = 0; k < m; k++)
				result[i][j] += A[k][i] * B[j][k];
		}
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Adapted from:
// https://www.geeksforgeeks.org/adjoint-inverse-matrix/


// Function to get cofactor of A[p][q] in temp[][]. n is current 
// dimension of A[][] 
void getCofactor(double A[][], double temp[][], int p, int q, int n){ 
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
double determinant(double A[][], int N){ 
	double D = 0; // Initialize result 
	
	//  Base case : if matrix contains single element 
	if (n == 1) 
		return A[0][0]; 
	
	// temp is used to store cofactors of A[][] 
	int sign = 1;
	double** temp = malloc(sizeof(double*) * N);
	for(int i = 0; i < N; i++)
		temp[i] = malloc(sizeof(double) * N);
	
	// Iterate for each element of first row 
	for (int f = 0; f < N; f++){ 
		// Getting Cofactor of A[0][f] 
		getCofactor(A, temp, 0, f, n); 
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
void adjoint(double A[][], double adj[][], int N){
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
void inverse(double A[][], double inverse[][], int N){ 
	// Find determinant of A[][] 
	double det = determinant(A, N); 
	if (det == 0) 
		return;
	
	// Find adjoint  
    adjoint(A, inverse); 
	
	// Find Inverse using formula "inverse(A) = adj(A)/det(A)" 
	for (int i = 0; i < N; i++) 
		for (int j = 0; j < N; j++) 
			inverse[i][j] /= det; 
	
	return; 
} 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}
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
void kalman_filter_step(struct state* curr, struct observe t0, struct observe t1, struct observe t2, double timestep){
	int i, j;
	struct state observation;
	observation.clk = curr->clk;
	observation.kinematics[AXIS_Y][DIST]  = t0.baro;
	observation.kinematics[AXIS_Y][VEL]   = (t0.baro - t1.baro) / timestep;
	observation.kinematics[AXIS_Y][ACCEL] = (t0.baro + t2.baro) / (timestep * timestep);
	
	observation.kinematics[AXIS_X][DIST]  = 0;
	observation.kinematics[AXIS_X][VEL]   = 0;
	observation.kinematics[AXIS_X][ACCEL] = 0;

	observation.kinematics[AXIS_Z][DIST]  = 0;
	observation.kinematics[AXIS_Z][VEL]   = 0;
	observation.kinematics[AXIS_Z][ACCEL] = 0;
	
	double clk;
//	double temperature;		// Temperature
//	double gyro[3][3];		// unit vectors for the axes of the accelerometer
//	double rot[3];			// Rate of rotation about each axis
	double kinematics[3][3];// Motion along each axis
	double p_k[3][3];		// Prediction error	
	// Prediction step:
	
	// Iterate through the axes
	for(i = 0; i < 3; i++){
		
		// Kinematics
		// Assume constant acceleration for now
		/*
		*		 [d(t)]   [1 t 0]	[d(t-1)]
		*	x_ = [v(t)] = [0 1 t] * [v(t-1)]
		*	 k	 [a(t)]   [0 0 1]	[a(t-1)]
		*/
		curr->kinematics[i][DIST] = curr->kinematics[i][DIST] + timestep * curr->kinematics[i][VEL];
		curr->kinematics[i][VEL] = curr->kinematics[i][VEL] + timestep * curr->kinematics[i][ACCEL];
	}
	
	// Prediction error
	// observation.p_k is a temp array
	matrix_mult(observation.p_k, CONST_A, curr->p_k, 3, 3, 3);
	matrix_mult(curr->p_k, observation.p_k, CONST_At, 3, 3, 3);


	
	// Update step:
	// x_hat gets x_hat + gain*(z-C*x_hat)
	// observation.p_k is a temp array
	// C * x_hat
	matrix_mult(observation.p_k, CONST_C, curr->kinematics, 3, 3, 3);
	
	// z - (C*x_hat)
	for(i = 0; i < 3; i++){
		for(j = 0; j < 3; j++)
			observation.kinematics[i][j] -= observation.p_k[i][j]; 
	}
	
	// gain*(z-C*x_hat)
	matrix_mult(observation.p_k, curr->gain, observation.kinematics, 3, 3, 3);
	
	// x_hat <= x_hat + gain*(z-C*x_hat)
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			curr->kinematics[i][j] += observation.p_k[i][j];
	
	
	// G_k=P_k*C_transpose*(C*P_k*C_(transpose)+R)^(−1)
	// holds inverse P_k + R
	for(i = 0; i < 3; i++)
		for(j = 0; j< 3; j++)
			observation.p_k[i][j] = p_k[i][j];
	
	// C*P-k*Ct
	matrix_mult(observation.gain, CONST_C, observation.p_k, 3, 3, 3);
	matrix_mult(observation.p_k, observation.gain, CONST_Ct, 3, 3, 3);
	
	// (C*P-k*Ct + R)
	// Assuming perfect instruments for now
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			observation.p_k[i][j] += 0;
		
	// Inversion, gain is temp
	inverse(observation.p_k, observation.gain, 3);
	
	// P_k
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			curr->gain[i][j] = curr->p_k[i][j];
	
	// P_k * Ct
	// gain = P_k*Ct*(C*P_k*Ct+R)^(-1)
	matrix_mult(observation.gain, curr->gain, CONST_Ct, 3, 3, 3);
	matrix_mult(curr->gain, observation.gain, observation.p_k, 3, 3, 3);
	
	
	// P_k <- (I - gain * C) * P_k

	// gain * C
	matrix_mult(observation.gain, curr->gain, CONST_C, 3, 3, 3);
	
	// I - (gain * C)
	// Copy P_k to temp
	for(i = 0; i < 3; i++){
		for(j = 0; j < 3; j++){
			observation.gain[i][j] = (i == j ? 1 : 0) - observation.gain[i][j];
			observation.p_k[i][j] = curr->p_k[i][j];
		}
	}
	matrix_mult(curr->p_k, observation.gain, observation.p_k, 3, 3, 3);
	
	return;
}

int main(){
	struct state* curr = malloc(sizeof(struct state));
	for(int i = 0; i < 3; i++){
		for(int j = 0; j < 3; j++){
			if(i != j){
				curr->gain[i][j] = 0.0;
				curr->p_k[i][j] = 0.0;
			} else {
				curr->gain[i][j] = 0.5;
				curr->p_k[i][j] = 0.5;
			}
		}
	}
	
	// Get data
	// Iterate data through filter
	// Write new data to file
	
	return 0;
}