#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>

#define PI 3.14159265

// curr is a 3x3 array representing a matrix with unit vectors for each accelerometer axis
// delta is a 3-element array representing the change in angle about the x-, y-, and z-axes in order
void update_alignment(double** curr, double* delta);