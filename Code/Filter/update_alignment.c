#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>

#define PI 3.14159265

// curr is a 3x3 array representing a matrix with unit vectors for each accelerometer axis
// delta is a 3-element array representing the change in angle about the x-, y-, and z-axes in order
void update_alignment(double** curr, double* delta){
	// Iterates over curr[]
	for(int i = 0; i < 3; i++){

		// Iterates over delta[]
		for(int j = 0; j < 3; j++){
			
			// Copy curr[i] so that we can change curr and still access it during rotation
			double rotating[3];
			for(int k = 0; k < 3; k++)
				rotating[k] = curr[i][k];
			
			// No effect rotating axis about itself, skip computation
			if(i == j)
				continue;
			
			switch(j){
				// Rotate about x axis
				case 0:
					// curr[i][0] = 
					curr[i][1] = rotating[1] * cos(delta[j]*PI/180) - rotating[2] * sin(delta[j]*PI/180);
					curr[i][2] = rotating[1] * sin(delta[j]*PI/180) + rotating[2] * cos(delta[j]*PI/180);
					break;
				
				// Rotate about y axis
				case 1:
					curr[i][0] = rotating[0] * cos(delta[j]*PI/180) + rotating[2] * sin(delta[j]*PI/180);
					// curr[i][1] = 
					curr[i][2] = -1 * rotating[0] * sin(delta[j]*PI/180) + rotating[2] * cos(delta[j]*PI/180);				
					break;
				
				// Rotate about z axis
				case 2:
					curr[i][0] = rotating[0] * cos(delta[j]*PI/180) - rotating[1] * sin(delta[j]*PI/180);
					curr[i][1] = rotating[0] * sin(delta[j]*PI/180) + rotating[1] * cos(delta[j]*PI/180);
					// curr[i][2] = 					
					break;
			}
		}
	}
	
	return;
}