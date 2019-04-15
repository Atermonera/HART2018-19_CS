The Kalman Filter is presently split into two implementations. filter.c represents the most modern development, which includes all observed and interpolated variables. filter_stable.c contains an older version processing just vertical motion, which was originally created to better compare tweaks against a working verson.
At present, filter_stable.c is preferable for actually testing the function of the filter, due mostly to a lack of foresight on my part. In a single-variable kalman filter, the algorithm is just a set of equations that can be run quite concisely. However, to expand the filter to track multiple variables, such as expanding an altitude filter to use velocity to predict the next state, one must use linear algebra, and represent everything in matrices.
This on its own isn't problematic, with just its 3 variables, filter_stable.c is able to process the input faster than the 0.01s timestep between simulated observations, allowing it to theoretically operate in real time. However, the multivariate kalman filter notably has a single matrix inversion operation, which is notably drastically more computationally intensive than the assorted matrix multiplications. As such, when I attempted to test the most recent version of filter.c, with its 22x22 matrix inversion, it appeared to hang until I added debug code and found that the point of contention was indeed the inversion.
I tried to run it overnight to see how far through the 24000 data points it could get, but unfortunately my computer went to sleep, closing the VPN session and terminating my PuTTY connection to the engr servers running the filter. As such, I do not know how long filter.c should actually take to complete a single step, and am interested if you dare to time that, but also note that you should plan on doing other things while it chugs in the background if you do test it.
In the coming weeks, I intend to refactor the kalman filter to accept variable sizes and then run several minimal filters for each "set" of related data (kinematics, perhaps split along each axis, gyroscope axes and rotation, temperature, pressure) to alleviate the load of matrix inversion. This will furthermore allow my to remove the constant-matrix declarations (Notably not constant for gyroscope axes due to the nature of their prediction step) from the filter itself, which should also optimize for time.

The explanation of the kalman filter's function will use filter.c's implementation. filter_stable.c has a simpler implementation consisting of only the vertical kinematics components, and the other data groups should similarly be isolatable.
The state struct is the operational data structure used by the kalman filter, storing the variables, process error, and gain. Each is a double**, which is in effect a matrix. The variables matrix is a 22x1 array containing, in order, X kinematics, Y kinematics, Z kinematics, Temperature, Rotation, and Gyroscopic axes. While this could be a double*, the use of the matrix multiplication functions requires a double**, does not introduce much operational complexity for an Nx1 output, and allows me to not largely copy-paste the code.
gain, the gain matrix, describes how each particular observed variable is related to each particular simulated variable. In theory, it allows for multiple observations, such as accelerometer and barometer, to measure the same variable, though I elected to combine those measurements according to a fixed gain into a single observation in order to keep the matrices square and reduce headache.
p_k, the process error matrix, describes how noisy the interpolated state is, which affects the computation of the gain. A low value for a particular index indicates that the first index is largely unaffected by the second, while a high value indicates that the first index is quite noisy on account of the second index. It functions more or less inversely to the gain. The key difference is that the gain is calculated at each step, whereas the error persists, and is rather updated at each step.
The clk variable just serves as an observation counter.

The observe struct represents the observable variables. The test function for the kalman filter takes the full dataset from the input files, and picks out what variables are actually observable. For instance, though the velocity is accurately tracked by the data generator, it cannot be directly observed by the rocket's instruments, and therefore must be interpolated from GPS, barometer, and accelerometer.
The kalman filter then interpolates the observed variables back into composite variables like velocity to present a more complete dataset and use square matrix multiplication within the function.

The init_state() and free_state() functions allocate and free dynamic memory used by the state struct.

The matrix_mult() function is a handwritten multiplication function for matrices of size NxM and MxP. The output will necessarily be overwritten because the multiplication accumulates results at each index, but must already be properly allocated.
The result parameter is the output matrix. The parameters A and B are the input matrices. The parameters n, m, and p represent the size of the input matrices. The verbose parameter is a debug option to print results as they are accumulated at each index.

The matrix_print() function outputs the contents of the supplied NxM matrix to stdout. It works a lot better when the matrix is small enough for a single row to print on a single line, which is most certainly not the case for most monitors when printing 22 doubles.
It prints row and column numbers with the matrices, for debugging purposes.

The readLine() function works like the getLine() function of <stdio.h>, but uses a file descriptor rather than a file pointer, and discards the newline "\n".
I wrote it for CS344, and prefer to use file descriptors to pointers. Were its uses to be appropriately switched with file pointers, it should function identically.

The matrix_transpose() function returns the MxN transpose of the supplied NxM matrix.

The functions const_A(), const_C,(), const_Q(), and const_R(), generate the constant matrices used by various steps of the kalman filter. For the most part, each are actually constant. However, due to the nature of the prediction step, const_A requires both the timestep and the current state.
const_A() describes the multiplicative relationship between each variable in the current state, and each variable in the next state. For example, the distance at the next timestep should be equal to the distance at the current timestep plus the current velocity multiplied by the change in time. This pattern is used for all the kinematics equations, but the matrix becomes much more complex when describing the gyroscopic axes.
In order to update the axes, we have to apply three rotation matrix transformations according to this:
	[1,0,0 ]   [c, 0,d]   [e,-f,0]   [x_i,y_i,z_i]
	[0,a,-b] * [0, 1,0] * [f,e, 0] * [x_j,y_j,z_j]
	[0,b,a ]   [-d,0,c]   [0,0, 1]   [x_k,y_k,z_k]
	
	Where [x_i, x_j, x_k] is a unit vector describing the x axis.
	
	Such that
	a = cos(curr->variables[ROTATION_X][0] * PI / 180)
	b = sin(curr->variables[ROTATION_X][0] * PI / 180)
	c = cos(curr->variables[ROTATION_Y][0] * PI / 180)
	d = sin(curr->variables[ROTATION_Y][0] * PI / 180)
	e = cos(curr->variables[ROTATION_Z][0] * PI / 180)
	f = sin(curr->variables[ROTATION_Z][0] * PI / 180)
	
	Expanding this with wolfram alpha, we arrive at the resulting matrix
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

This can then be expanded according to the definition of each of these constants into the resulting mess of sin()s and cos()s. Unfortunately, the kalman filter cannot use the update_alignment() function I wrote for the data generation, which itself is a simplified matrix multiplication according to fixed matrices, because the kalman filter requires that all observed variables be in an Nx1 matrix, and update_alignment() requires a 3x3 specifically for the unit vectors.
Furthermore, I cannot extract the unit vectors, use update_alignment(), and insert them back into the matrix, because CONST_A is used for more than just updating the predicted state. To use another method to update a section of the variables would violate the purpose of the filter at large.
This was a headache to try and figure out, and absolutely one of the reasons that I implemented filter_stable.c first.

const_C() is much simpler, and is as expected an identity matrix, describing the multiplicative variation of the observed state from the actual state, described by:
z_k = C * x_k + v_k
Where z_k is the observation, x_k the real state, and v_k instrument noise.
const_Q() describes a process noise, and essentially prevents the predictions from thinking that they are too accurate by introducing further noise so that we are required to have a non-0 p_k and therefore a real gain (Inverting a 0 matrix is technically impossible, and results in very large values due to floating point error (I think)).
const_R() describes the variance of each observation with respect to each measured variable. The used values are actually arbitrary, because the ECE subteam hasn't given me real variances. It is possible that CONST_R will actually need to be dynamically calculated in the same manner as A, if particular instruments work better or worse in particular conditions (Such as the thermometer reading incorrectly if the temperature exceeds ~180F).

The getCofactor(), determinant(), adjoint(), and inverse() functions perform the matrix inversion necessary for the update step. They were found online, because it's been too long since I've done such complex matrix operations to want to think about how to even approach it programmatically. There's a link to the source in a comment above getCofactor(). I did make some tweaks, mostly to use C instead of C++.

The kalman_filter_step() function performs the meat of the kalman filter's work. The curr parameter contains the current state, and is updated to produce the next state. Recording each state must be performed by the calling function, whatever form that takes. The parameters t0, t1, and t2 describe the observations at the current, last, and second-last timesteps respectively. The verbose, debug, and crash parameters are each flags that determine stdout logging behaviour. 
Verbose will print observation and state matrices, as well as how they have updated at the end of the prediction and update steps. Debug will print intermediate steps in the generally complex operations of the update step. Crash will cause a forceable exit() when the step completes, and is used to halt the function of the filter at a prescribed step. The usage for these three parameters is generally a set of ternary operators that will switch them on after a certain step count is reached.
The kalman filter interpolates the various kinematics variables according to a rather convoluted and entirely arbitrary ratio based upon perceived reliability of one particular measurement over another. It also switches on the availability of GPS, because the GPS operates on a much slower clock.
It then sets up the constant matrices and their transposes, and creates a temporary variables-matrix. The prediction step consists of two equations, as follows:
	^x(k) = A * ^x(k-1)
	p(k) = A * p(k-1) * At + Q

Where ^x(k) is the predicted state at time k, p(k) is the process error at time k, and At is the transpose of A.
The update step factors the observed state z(k) into the predicted state, and adjusts p(k).
	^x(k) <- ^x(k) + gain * (z - C * ^x(k))
	gain = p(k) * Ct * inverse(C * p(k) * Ct + R)
	p(k) <- (I - gain * C) * p(k)
	
Where Ct is the transpose of C, I is an identity matrix.
The matrix inversion step is the point of failure for filter.c, because it's such a computationall complex algorithm. Each equation is broken into multiple steps of multiplication and addition, because matrix multiplication requires a function call, and addition requires iterating over every index.
Once the update step has conclude, there's a sanity check to prevent the state from going underground, and forceably halting motion and rotation once an altitude of <=0 is reached. kalman_filter_step then frees allocated memory and returns.

The main() function serves to read data in from the file output of the data generator, parse it into variables, and then give each observation to the kalman filter. The file input and output are hardcoded.
There may be an infinite loop condition in filter_stable.c, because I hadn't properly coded exit conditions for when EOF is read by readLine(). If that is the case, when the program stops outputting a step count, it has read and processed all steps, and can be stopped.