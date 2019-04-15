#include "datagen.h"

#include <errno.h>

#define PI 3.14159265

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
#define TEMPERATURE		9
#define ROTATION_X		10
#define ROTATION_Y		11
#define ROTATION_Z		12
#define GYRO_X_i		13
#define GYRO_X_j		14
#define GYRO_X_k		15
#define GYRO_Y_i		16
#define GYRO_Y_j		17
#define GYRO_Y_k		18
#define GYRO_Z_i		19
#define GYRO_Z_j		20
#define GYRO_Z_k		21

#define VERBOSE 1
#define DEBUG 1
#define CRASH 1

// Predicted state
struct state{
	double clk;
	double** variables;		// Contains all measured variables
	double** p_k;			// Prediction error
	double** gain;			// Gain
};

// Set of observations for timestep k
struct observe{
	double gyro[3];		// Rotation about gyro axes
	double accel[3];	// Acceleration along fixed axes
	double gps[3];		// Not always available due to timing
	double baro;		// Barometric altitude
	double temp;		// Temperature
	int gps_set;		// flag
};

void init_state(struct state* S){
	S->variables = malloc(sizeof(double*) * 22);
	S->p_k = malloc(sizeof(double*) * 22);
	S->gain = malloc(sizeof(double*) * 22);
	
	for(int i = 0; i < 22; i++){
		S->p_k[i] = malloc(sizeof(double) * 22);
		S->gain[i] = malloc(sizeof(double) * 22);
		S->variables[i] = malloc(sizeof(double) * 1);
	}
	
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
double** const_A(struct state* curr, double timestep){
	double** m = malloc(sizeof(double*) * 22);
	for(int i = 0; i < 22; i++){
		m[i] = malloc(sizeof(double) * 22);
		for(int j = 0; j < 22; j++)
			m[i][j] = 0;
	}
	
	// Manual index assignment for non-zero values
	// X axis
	m[AXIS_X_DIST][AXIS_X_DIST] 	= 1;
	m[AXIS_X_VEL][AXIS_X_DIST] 		= timestep;
	m[AXIS_X_VEL][AXIS_X_VEL] 		= 1;
	m[AXIS_X_ACCEL][AXIS_X_VEL] 	= timestep;
	m[AXIS_X_ACCEL][AXIS_X_ACCEL] 	= 1;
	// Y axis
	m[AXIS_Y_DIST][AXIS_Y_DIST] 	= 1;
	m[AXIS_Y_VEL][AXIS_Y_DIST] 		= timestep;
	m[AXIS_Y_VEL][AXIS_Y_VEL] 		= 1;
	m[AXIS_Y_ACCEL][AXIS_Y_VEL] 	= timestep;
	m[AXIS_Y_ACCEL][AXIS_Y_ACCEL] 	= 1;
	// Z axis
	m[AXIS_Z_DIST][AXIS_Z_DIST] 	= 1;
	m[AXIS_Z_VEL][AXIS_Z_DIST] 		= timestep;
	m[AXIS_Z_VEL][AXIS_Z_VEL] 		= 1;
	m[AXIS_Z_ACCEL][AXIS_Z_VEL] 	= timestep;
	m[AXIS_Z_ACCEL][AXIS_Z_ACCEL] 	= 1;
	// Temperature
	m[TEMPERATURE][TEMPERATURE]		= 1;
	// Rotation
	m[ROTATION_X][ROTATION_X] 		= 1;
	m[ROTATION_Y][ROTATION_Y] 		= 1;
	m[ROTATION_Z][ROTATION_Z] 		= 1;
	// Gyro unit vectors
	/************************************************************************
	Have to apply three rotation matrix transformations according to this:
	[1,0,0 ]   [c, 0,d]   [e,-f,0]   [x_i,y_i,z_i]
	[0,a,-b] * [0, 1,0] * [f,e, 0] * [x_j,y_j,z_j]
	[0,b,a ]   [-d,0,c]   [0,0, 1]   [x_k,y_k,z_k]
	
	Such that
	a = cos(curr->variables[ROTATION_X][0] * PI / 180)
	b = sin(curr->variables[ROTATION_X][0] * PI / 180)
	c = cos(curr->variables[ROTATION_Y][0] * PI / 180)
	d = sin(curr->variables[ROTATION_Y][0] * PI / 180)
	e = cos(curr->variables[ROTATION_Z][0] * PI / 180)
	f = sin(curr->variables[ROTATION_Z][0] * PI / 180)
	
	Expanding this, we arrive at
	[		ce*x_i - cf*x_j + d*x_k					ce*y_i - cf*y_j + d*y_k					ce*z_i - cf*z_j + d*z_k		]
	[(bde+af)*x_i + (ae-bdf)*x_j - bc*x_k	(bde+af)*y_i + (ae-bdf)*y_j - bc*y_k	(bde+af)*z_i + (ae-bdf)*z_j - bc*z_k]
	[(bf-ade)*x_i + (be+adf)*x_j + ac*x_k	(bf-ade)*y_i + (be+adf)*y_j + ac*y_k	(bf-ade)*z_i + (be+adf)*z_j + ac*z_k]
	
	Breaking this into the format of Const_A, we find
	m[GYRO_X_i][GYRO_X_i] = ce;
	m[GYRO_X_i][GYRO_X_j] = -cf;
	m[GYRO_X_i][GYRO_X_k] = d;
	
	m[GYRO_X_j][GYRO_X_i] = af+bde;
	m[GYRO_X_j][GYRO_X_j] = ae-bdf;
	m[GYRO_X_j][GYRO_X_k] = -bc;
	
	m[GYRO_X_k][GYRO_X_i] = bf-ade;
	m[GYRO_X_k][GYRO_X_j] = be+adf;
	m[GYRO_X_k][GYRO_X_k] = ac;
	
	m[GYRO_Y_i][GYRO_Y_i] = ce;
	m[GYRO_Y_i][GYRO_Y_j] = -cf;
	m[GYRO_Y_i][GYRO_Y_k] = d;
	
	m[GYRO_Y_j][GYRO_Y_i] = af+bde;
	m[GYRO_Y_j][GYRO_Y_j] = ae-bdf;
	m[GYRO_Y_j][GYRO_Y_k] = -bc;
	
	m[GYRO_Y_k][GYRO_Y_i] = bf-ade;
	m[GYRO_Y_k][GYRO_Y_j] = be+adf;
	m[GYRO_Y_k][GYRO_Y_k] = ac;
	
	m[GYRO_Z_i][GYRO_Z_i] = ce;
	m[GYRO_Z_i][GYRO_Z_j] = -cf;
	m[GYRO_Z_i][GYRO_Z_k] = d;
	
	m[GYRO_Z_j][GYRO_Z_i] = af+bde;
	m[GYRO_Z_j][GYRO_Z_j] = ae-bdf;
	m[GYRO_Z_j][GYRO_Z_k] = -bc;
	
	m[GYRO_Z_k][GYRO_Z_i] = bf-ade;
	m[GYRO_Z_k][GYRO_Z_j] = be+adf;
	m[GYRO_Z_k][GYRO_Z_k] = ac;
		
	Replacing constants with their real values,
	We find this section of Const_A to contain:
	************************************************************************/
	m[GYRO_X_i][GYRO_X_i] =  (cos(curr->variables[ROTATION_Y][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_X_i][GYRO_X_j] = -(cos(curr->variables[ROTATION_Y][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_X_i][GYRO_X_k] =   sin(curr->variables[ROTATION_Y][0] * PI / 180);
	
	m[GYRO_X_j][GYRO_X_i] =  (cos(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180)) + (sin(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_X_j][GYRO_X_j] =  (cos(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180)) - (sin(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_X_j][GYRO_X_k] = -(sin(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Y][0] * PI / 180));
	
	m[GYRO_X_k][GYRO_X_i] =  (sin(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180)) - (cos(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_X_k][GYRO_X_j] =  (sin(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180)) + (cos(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_X_k][GYRO_X_k] =  (cos(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Y][0] * PI / 180));
	
	m[GYRO_Y_i][GYRO_Y_i] =  (cos(curr->variables[ROTATION_Y][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Y_i][GYRO_Y_j] = -(cos(curr->variables[ROTATION_Y][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Y_i][GYRO_Y_k] =   sin(curr->variables[ROTATION_Y][0] * PI / 180);
	
	m[GYRO_Y_j][GYRO_Y_i] =  (cos(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180)) + (sin(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Y_j][GYRO_Y_j] =  (cos(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180)) - (sin(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Y_j][GYRO_Y_k] = -(sin(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Y][0] * PI / 180));
	
	m[GYRO_Y_k][GYRO_Y_i] =  (sin(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180)) - (cos(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Y_k][GYRO_Y_j] =  (sin(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180)) + (cos(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Y_k][GYRO_Y_k] =  (cos(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Y][0] * PI / 180));
	
	m[GYRO_Z_i][GYRO_Z_i] =  (cos(curr->variables[ROTATION_Y][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Z_i][GYRO_Z_j] = -(cos(curr->variables[ROTATION_Y][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Z_i][GYRO_Z_k] =   sin(curr->variables[ROTATION_Y][0] * PI / 180);
	
	m[GYRO_Z_j][GYRO_Z_i] =  (cos(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180)) + (sin(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Z_j][GYRO_Z_j] =  (cos(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180)) - (sin(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Z_j][GYRO_Z_k] = -(sin(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Y][0] * PI / 180));
	
	m[GYRO_Z_k][GYRO_Z_i] =  (sin(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180)) - (cos(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Z_k][GYRO_Z_j] =  (sin(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Z][0] * PI / 180)) + (cos(curr->variables[ROTATION_X][0] * PI / 180) * sin(curr->variables[ROTATION_Y][0] * PI / 180) * sin(curr->variables[ROTATION_Z][0] * PI / 180));
	m[GYRO_Z_k][GYRO_Z_k] =  (cos(curr->variables[ROTATION_X][0] * PI / 180) * cos(curr->variables[ROTATION_Y][0] * PI / 180));
	
	return m;
}

double** const_C(){
	double** m = malloc(sizeof(double*) * 22);
	for(int i = 0; i < 22; i++){
		m[i] = malloc(sizeof(double) * 22);
		for(int j = 0; j < 22; j++)
			m[i][j] = 0;
	}
	
	// Manual index assignment for non-zero values
	// X axis
	m[AXIS_X_DIST][AXIS_X_DIST] 	= 1;
	m[AXIS_X_VEL][AXIS_X_VEL] 		= 1;
	m[AXIS_X_ACCEL][AXIS_X_ACCEL] 	= 1;
	// Y axis
	m[AXIS_Y_DIST][AXIS_Y_DIST] 	= 1;
	m[AXIS_Y_VEL][AXIS_Y_VEL] 		= 1;
	m[AXIS_Y_ACCEL][AXIS_Y_ACCEL] 	= 1;
	// Z axis
	m[AXIS_Z_DIST][AXIS_Z_DIST] 	= 1;
	m[AXIS_Z_VEL][AXIS_Z_VEL] 		= 1;
	m[AXIS_Z_ACCEL][AXIS_Z_ACCEL] 	= 1;
	// Temperature
	m[TEMPERATURE][TEMPERATURE]		= 1;
	// Rotation
	m[ROTATION_X][ROTATION_X] 		= 1;
	m[ROTATION_Y][ROTATION_Y] 		= 1;
	m[ROTATION_Z][ROTATION_Z] 		= 1;
	// Gyro unit vectors
	m[GYRO_X_i][GYRO_X_i] 			= 1;
	m[GYRO_X_j][GYRO_X_j] 			= 1;
	m[GYRO_X_k][GYRO_X_k] 			= 1;
	m[GYRO_Y_i][GYRO_Y_i] 			= 1;
	m[GYRO_Y_j][GYRO_Y_j] 			= 1;
	m[GYRO_Y_k][GYRO_Y_k] 			= 1;
	m[GYRO_Z_i][GYRO_Z_i] 			= 1;
	m[GYRO_Z_j][GYRO_Z_j] 			= 1;
	m[GYRO_Z_k][GYRO_Z_k] 			= 1;
	
	return m;
}

double** const_Q(){
	double** m = malloc(sizeof(double*) * 22);
	for(int i = 0; i < 22; i++){
		m[i] = malloc(sizeof(double) * 22);
		for(int j = 0; j < 22; j++)
			m[i][j] = (i == j ? 1 : 0);
	}
	
	// Manual index assignment for non-zero values
	// X axis
	m[AXIS_X_DIST][AXIS_X_DIST] 	= 0.01;
	m[AXIS_X_VEL][AXIS_X_VEL] 		= 0.01;
	m[AXIS_X_ACCEL][AXIS_X_ACCEL] 	= 0.01;
	// Y axis
	m[AXIS_Y_DIST][AXIS_Y_DIST] 	= 0.01;
	m[AXIS_Y_VEL][AXIS_Y_VEL] 		= 0.01;
	m[AXIS_Y_ACCEL][AXIS_Y_ACCEL] 	= 0.01;
	// Z axis
	m[AXIS_Z_DIST][AXIS_Z_DIST] 	= 0.01;
	m[AXIS_Z_VEL][AXIS_Z_VEL] 		= 0.01;
	m[AXIS_Z_ACCEL][AXIS_Z_ACCEL] 	= 0.01;
	// Temperature
	m[TEMPERATURE][TEMPERATURE]		= 0.01;
	// Rotation
	m[ROTATION_X][ROTATION_X] 		= 0.01;
	m[ROTATION_Y][ROTATION_Y] 		= 0.01;
	m[ROTATION_Z][ROTATION_Z] 		= 0.01;
	// Gyro unit vectors
	m[GYRO_X_i][GYRO_X_i] 			= 0.01;
	m[GYRO_X_j][GYRO_X_j] 			= 0.01;
	m[GYRO_X_k][GYRO_X_k] 			= 0.01;
	m[GYRO_Y_i][GYRO_Y_i] 			= 0.01;
	m[GYRO_Y_j][GYRO_Y_j] 			= 0.01;
	m[GYRO_Y_k][GYRO_Y_k] 			= 0.01;
	m[GYRO_Z_i][GYRO_Z_i] 			= 0.01;
	m[GYRO_Z_j][GYRO_Z_j] 			= 0.01;
	m[GYRO_Z_k][GYRO_Z_k] 			= 0.01;
	
	return m;
}

double** const_R(){
	double** m = malloc(sizeof(double*) * 22);
	for(int i = 0; i < 22; i++){
		m[i] = malloc(sizeof(double) * 22);
		for(int j = 0; j < 22; j++)
			m[i][j] = (i == j ? 1 : 0);
	}
	
	// Manual index assignment for non-zero values
	// X axis
	m[AXIS_X_DIST][AXIS_X_DIST] 	= 0.8;
	m[AXIS_X_VEL][AXIS_X_VEL] 		= 0.6;
	m[AXIS_X_ACCEL][AXIS_X_ACCEL] 	= 0.4;
	// Y axis
	m[AXIS_Y_DIST][AXIS_Y_DIST] 	= 0.4;
	m[AXIS_Y_VEL][AXIS_Y_VEL] 		= 0.6;
	m[AXIS_Y_ACCEL][AXIS_Y_ACCEL] 	= 0.4;
	// Z axis
	m[AXIS_Z_DIST][AXIS_Z_DIST] 	= 0.8;
	m[AXIS_Z_VEL][AXIS_Z_VEL] 		= 0.6;
	m[AXIS_Z_ACCEL][AXIS_Z_ACCEL] 	= 0.4;
	// Temperature
	m[TEMPERATURE][TEMPERATURE]		= 0.1;
	// Rotation
	m[ROTATION_X][ROTATION_X] 		= 0.1;
	m[ROTATION_Y][ROTATION_Y] 		= 0.1;
	m[ROTATION_Z][ROTATION_Z] 		= 0.1;
	// Gyro unit vectors
	m[GYRO_X_i][GYRO_X_i] 			= 0.01;
	m[GYRO_X_j][GYRO_X_j] 			= 0.01;
	m[GYRO_X_k][GYRO_X_k] 			= 0.01;
	m[GYRO_Y_i][GYRO_Y_i] 			= 0.01;
	m[GYRO_Y_j][GYRO_Y_j] 			= 0.01;
	m[GYRO_Y_k][GYRO_Y_k] 			= 0.01;
	m[GYRO_Z_i][GYRO_Z_i] 			= 0.01;
	m[GYRO_Z_j][GYRO_Z_j] 			= 0.01;
	m[GYRO_Z_k][GYRO_Z_k] 			= 0.01;
	
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
	// Without gps, we interpolate lateral position from acceleration only
	if(!t0.gps_set){
		// X axis
		observation.variables[AXIS_X_ACCEL][0] = t0.accel[0];
		observation.variables[AXIS_X_VEL][0] = curr->variables[AXIS_X_VEL][0] + t0.accel[0] * timestep;
		observation.variables[AXIS_X_DIST][0] = curr->variables[AXIS_X_DIST][0] + observation.variables[AXIS_X_VEL][0] * timestep;

		// Y axis
		observation.variables[AXIS_Y_DIST][0]  = t0.baro;
		observation.variables[AXIS_Y_VEL][0]   = 0.5 * ((t0.baro - t1.baro) / timestep) + 0.5 * (t0.accel[2] * timestep + (t1.baro - t2.baro) / timestep); // Split between baro and accelerometer
		observation.variables[AXIS_Y_ACCEL][0] = 0.1 * (t0.baro - 2 * t1.baro + t2.baro) / (timestep * timestep) + 0.9 * t0.accel[2]; // Heavily weighted towards accelerometer
		
		// Z axis
		observation.variables[AXIS_Z_ACCEL][0] = t0.accel[2];
		observation.variables[AXIS_Z_VEL][0] = curr->variables[AXIS_Z_VEL][0] + t0.accel[2] * timestep;
		observation.variables[AXIS_Z_DIST][0] = curr->variables[AXIS_Z_DIST][0] + observation.variables[AXIS_Z_VEL][0] * timestep;
	
	} else {
		// If we have GPS, then we want to favor its measurements, because they're probably more accurate than whatever else we used to measure position
		// X axis
		observation.variables[AXIS_X_ACCEL][0] = t0.accel[0];
		observation.variables[AXIS_X_VEL][0] = 0.5 * (curr->variables[AXIS_X_VEL][0] + t0.accel[0] * timestep) + 0.5 * (t0.gps[0] - curr->variables[AXIS_X_DIST][0]) / timestep; // Split between gps and acceleromenter
		observation.variables[AXIS_X_DIST][0] = 0.9 * t0.gps[0] + 0.1 * (curr->variables[AXIS_X_DIST][0] + observation.variables[AXIS_X_VEL][0] * timestep); // Heavily weighted towards gps

		// Y axis
		observation.variables[AXIS_Y_DIST][0]  = 0.2 * t0.baro + 0.8 * t0.gps[1]; // Weighted towards gps
		observation.variables[AXIS_Y_VEL][0]   = 0.5 * ((0.2 * t0.baro + 0.8 * t0.gps[1] - t1.baro) / timestep) + 0.5 * (t0.accel[2] * timestep + (t1.baro - t2.baro) / timestep); // Split between baro
		observation.variables[AXIS_Y_ACCEL][0] = 0.1 * (0.2 * t0.baro + 0.8 * t0.gps[1] - 2 * t1.baro + t2.baro) / (timestep * timestep) + 0.9 * t0.accel[2]; // Heavily weighted towards accelerometer
		
		// Z axis
		observation.variables[AXIS_Z_ACCEL][0] = t0.accel[2];
		observation.variables[AXIS_Z_VEL][0] = 0.5 * (curr->variables[AXIS_Z_VEL][0] + t0.accel[2] * timestep) + 0.5 * (t0.gps[2] - curr->variables[AXIS_Z_DIST][0]) / timestep; // Split between gps and acceleromenter
		observation.variables[AXIS_Z_DIST][0] = 0.9 * t0.gps[2] + 0.1 * (curr->variables[AXIS_Z_DIST][0] + observation.variables[AXIS_Z_VEL][0] * timestep); // Heavily weighted towards gps
	}
	
/*	
	if(verbose){
		printf("\n***********************\n");
		printf("DEBUG: Observation translation\n");
		printf("Altitude: %lf = %lf\n", observation.variables[AXIS_Y_DIST], t0.baro);
		printf("Velocity: %lf = (%lf - %lf) / %lf\n", observation.variables[AXIS_Y_VEL], t0.baro, t1.baro, timestep);
		printf("Accelera: %lf = (%lf - %lf) / (%lf * %lf)", observation.variables[AXIS_Y_ACCEL], t0.baro, t2.baro, timestep, timestep);
		printf("\n***********************\n");	}
 */	

	
	for(int i = 0; i < 22; i++){
		for(int j = 0; j < 22; j++){
			observation.p_k[i][j] = 0;
			observation.gain[i][j] = 0;
		}
	}
	
	if(verbose){
		printf("\n***********************\n");
		printf("INFO: Observation\n");
		matrix_print(observation.variables, 22, 1);
		printf("\n***********************\n");
	}
	
//	double temperature;		// Temperature
//	double gyro[3][3];		// unit vectors for the axes of the accelerometer
//	double rot[3];			// Rate of rotation about each axis
//	double kinematics[3][3];// Motion along each axis
//	double p_k[3][3];		// Prediction error	
	double** CONST_A = const_A(curr, timestep);
	double** CONST_At = matrix_transpose(CONST_A, 22, 22);
	double** CONST_C = const_C();
	double** CONST_Ct = matrix_transpose(CONST_C, 22, 22);
	double** CONST_R = const_R();
	double** CONST_Q = const_Q();
	double** temp = malloc(sizeof(double*) * 22);
	for(int i = 0; i < 22; i++)
		temp[i] = malloc(sizeof(double) * 1);
	
	if(debug){
		printf("\n******************\n");
		printf("DEBUG: CONSTANT MATRICES\n\n");
		printf("A:\n");
		matrix_print(CONST_A, 22, 22);
		printf("\nAt:\n");
		matrix_print(CONST_At, 22, 22);
		printf("\nC:\n");
		matrix_print(CONST_C, 22, 22);
		printf("\nCt:\n");
		matrix_print(CONST_Ct, 22, 22);
		printf("\n******************\n");
	}
	if(verbose){
		printf("\nINFO: VARIABLE MATRICES\n\n");
		printf("Variables:\n");
		matrix_print(curr->variables, 22, 1);
		printf("\nGain:\n");
		matrix_print(curr->gain, 22, 22);
		printf("\np_k:\n");
		matrix_print(curr->p_k, 22, 22);
		printf("\n*******************\n");
	} 
	
	// Prediction step:
	matrix_mult(temp, CONST_A, curr->variables, 22, 22, 1, 0);
		
	// Prediction error
	// observation.p_k is a temp array
	// P_k = A * P_(k-1) * At + Q
	matrix_mult(observation.p_k, CONST_A, curr->p_k, 22, 22, 22, 0);
	matrix_mult(curr->p_k, observation.p_k, CONST_At, 22, 22, 22, 0);

	for(int i = 0; i < 22; i++){
		curr->variables[i][0] = temp[i][0]; // Have to move the predicted variables back into curr->variables
		for(int j = 0; j < 22; j++)
			curr->p_k[i][j] += CONST_Q[i][j];
	}
	if(verbose){
		printf("\n******************\n");
		printf("INFO: PREDICTION STEP\n\n");
		printf("Variables:\n");
		matrix_print(curr->variables, 22, 1);
		printf("\np_k:\n");
		matrix_print(curr->p_k, 22, 22);
		printf("\n******************\n");
	}
	
	if(debug)
		printf("\nDEBUG: UPDATE STEP\n\n");
	
	// Update step:
	// x_hat gets x_hat + gain*(z-C*x_hat)
	// C * x_hat
	matrix_mult(temp, CONST_C, curr->variables, 22, 22, 1, 0);
	
	if(debug){
		printf("\nCONST_C * X_hat:\n");
		matrix_print(temp, 22, 1);
	}
	
	// z - (C*x_hat)
	for(int i = 0; i < 22; i++){
		observation.variables[i][0] -= temp[i][0];
	}
	
	if(debug){
		printf("\nZ - (CONST_C * X_hat):\n");
		matrix_print(observation.variables, 22, 1);
	}	
	
	// gain*(z-C*x_hat)
	matrix_mult(temp, curr->gain, observation.variables, 22, 22, 1, 0);
	
	if(debug){
		printf("\ngain * (Z - (CONST_C * X_hat)):\n");
		matrix_print(temp, 22, 1);
	}	
	
	// x_hat <= x_hat + gain*(z-C*x_hat)
	for(i = 0; i < 22; i++)
		curr->variables[i][0] += temp[i][0];
	
	if(debug){
		printf("\nX_hat <= X_hat + gain * (Z - (CONST_C * X_hat)):\n");
		matrix_print(curr->variables, 22, 1);
	}	
	
	// G_k=P_k*C_transpose*(C*P_k*C_(transpose)+R)^(−1)
	// holds inverse P_k
	for(i = 0; i < 22; i++)
		for(j = 0; j< 22; j++)
			observation.p_k[i][j] = curr->p_k[i][j];
	
	if(debug){
		printf("\np_k + R:\n");
		matrix_print(observation.p_k, 22, 22);
	}		
	
	// C*P_k*Ct
	matrix_mult(observation.gain, CONST_C, observation.p_k, 22, 22, 22, 0);
	matrix_mult(observation.p_k, observation.gain, CONST_Ct, 22, 22, 22, 0);

	// (C*P-k*Ct + R)
	for(i = 0; i < 22; i++)
		for(j = 0; j< 22; j++)
			observation.p_k[i][j] += CONST_R[i][j];
	
	if(debug){
		printf("\nCONST_C * p_k * CONST_Ct + R:\n");
		matrix_print(observation.p_k, 22, 22);
	}	
		
	// Inversion, gain is temp
	inverse(observation.p_k, observation.gain, 22);

	if(debug){
		printf("\ninverse(CONST_C * p_k * CONST_Ct + R):\n");
		matrix_print(observation.gain, 22, 22);
	}	

	
	// P_k
	for(i = 0; i < 22; i++)
		for(j = 0; j < 22; j++)
			curr->gain[i][j] = curr->gain[i][j];
	
	// P_k * Ct
	// gain = P_k*Ct*(C*P_k*Ct+R)^(-1)
	matrix_mult(observation.p_k, curr->p_k, CONST_Ct, 22, 22, 22, 0);
	matrix_mult(curr->gain, observation.p_k, observation.gain, 22, 22, 22, 0);
	
	if(debug){
		printf("\ngain = P_k*Ct*inverse(C*P_k*Ct+R):\n");
		matrix_print(curr->gain, 22, 22);
	}		
	
	// P_k <- (I - gain * C) * P_k

	// gain * C
	matrix_mult(observation.gain, curr->gain, CONST_C, 22, 22, 22, 0);
	
	// I - (gain * C)
	// Copy P_k to temp
	for(i = 0; i < 22; i++){
		for(j = 0; j < 22; j++){
			observation.gain[i][j] = (i == j ? 1 : 0) - observation.gain[i][j];
			observation.p_k[i][j] = curr->p_k[i][j];
		}
	}
	matrix_mult(curr->p_k, observation.gain, observation.p_k, 22, 22, 22, 0);
	
	if(debug){
		printf("\nP_k <- (I - gain * C) * P_k:\n");
		matrix_print(curr->p_k, 22, 22);
	}
	
	// SANITY CHECK
	// No motion once we've landed
	if(curr->variables[AXIS_Y_DIST] < 0){
		curr->variables[AXIS_X_VEL][0] = 	0;
		curr->variables[AXIS_X_ACCEL][0] = 	0;
		curr->variables[AXIS_Y_DIST][0] = 	0;
		curr->variables[AXIS_Y_VEL][0] = 	0;
		curr->variables[AXIS_Y_ACCEL][0] = 	0;
		curr->variables[AXIS_Z_VEL][0] = 	0;
		curr->variables[AXIS_Z_ACCEL][0] = 	0;
		curr->variables[ROTATION_X][0] = 	0;
		curr->variables[ROTATION_Y][0] = 	0;
		curr->variables[ROTATION_Z][0] = 	0;		
	}

	
	
	
	for(int i = 0; i < 22; i++){
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
	for(int i = 0; i < 22; i++){
		curr->variables[i][0] = 0.0;
		for(int j = 0; j < 22; j++){
			curr->gain[i][j] = 0.0;
			curr->p_k[i][j] = 0.0;
			// Initialize error and gain
			if(i == j){
				curr->gain[i][j] = 1;
				curr->p_k[i][j] = 1;
			}		
		}
	}
	
	struct observe t0, t1, t2;
	t0.baro = 0;
	t1.baro = 0;
	t2.baro = 0;
	t0.gps_set = 0; // No gps simulation in the test function
	t1.gps_set = 0;
	t2.gps_set = 0;
		
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
	int count = 0;
	
	printf("Files open\n");
	
	// Iterate data through filter
	// readLine returns 0 on EOF
	while(readLine(line, fp)){
		printf("Iteration %d\t read %d: %s\n", count, strlen(line), line);
		sscanf(line, "%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf\n", &(observation->clk), &(observation->variables[AXIS_X_DIST][0]), &(observation->variables[AXIS_X_VEL][0]), &(observation->variables[AXIS_X_ACCEL][0]), &(observation->variables[AXIS_Y_DIST][0]), &(observation->variables[AXIS_Y_VEL][0]), &(observation->variables[AXIS_Y_ACCEL][0]), &(observation->variables[AXIS_Z_DIST][0]), &(observation->variables[AXIS_Z_VEL][0]), &(observation->variables[AXIS_Z_ACCEL][0]), &(observation->variables[TEMPERATURE][0]), &(observation->variables[GYRO_X_i][0]), &(observation->variables[GYRO_X_j][0]), &(observation->variables[GYRO_X_k][0]), &(observation->variables[GYRO_Y_i][0]), &(observation->variables[GYRO_Y_j][0]), &(observation->variables[GYRO_Y_k][0]), &(observation->variables[GYRO_Z_i][0]), &(observation->variables[GYRO_Z_j][0]), &(observation->variables[GYRO_Z_k][0]), &(observation->variables[ROTATION_X][0]), &(observation->variables[ROTATION_Y][0]), &(observation->variables[ROTATION_Z][0]));
		memset(line, '\0', sizeof(line));
		
		printf("Line parsed\n");
		
		// Update the observations
		t2.baro = t1.baro;
		t1.baro = t0.baro;
		t0.baro = observation->variables[AXIS_Y_DIST][0];
		for(int i = 0; i < 3; i++){
			t2.gyro[i]  = t1.gyro[i];
			t1.gyro[i]  = t0.gyro[i];
			t2.accel[i] = t1.accel[i];
			t1.accel[i] = t0.accel[i];
		}
		t0.gyro[0]  = observation->variables[ROTATION_X][0];
		t0.gyro[1]  = observation->variables[ROTATION_Y][0];
		t0.gyro[2]  = observation->variables[ROTATION_Z][0];
		t0.accel[0] = observation->variables[AXIS_X_ACCEL][0];
		t0.accel[1] = observation->variables[AXIS_Y_ACCEL][0];
		t0.accel[2] = observation->variables[AXIS_Z_ACCEL][0];
		t2.temp = t1.temp;
		t1.temp = t0.temp;
		t0.temp = observation->variables[TEMPERATURE][0];
		printf("Analyzing step %d\n", count++);
		kalman_filter_step(curr, t0, t1, t2, 0.01, 0, 0, 0);
	//	kalman_filter_step(curr, t0, t1, t2, 0.01, count > -1 ? VERBOSE : 0, count > -1 ? DEBUG : 0, count > 2 ? CRASH : 0);
		
		
	//	printf("\n************************************************\np_k:\n");
	//	matrix_print(curr->p_k, 3, 3);
	//	printf("\n************************************************\n");

		
		// Write new data to file
		char buffer[1024];
		memset(buffer, '\0', sizeof(buffer));
		sprintf(buffer, "%f, %f, %f, %f\n", curr->clk, curr->variables[AXIS_Y_DIST][0], curr->variables[AXIS_Y_VEL][0], curr->variables[AXIS_Y_ACCEL][0]);
		write(output, buffer, strlen(buffer));
	}
	
	free_state(curr);
	free_state(observation);
	// Close files
	close(fp);
	close(output);
	return 0;
}