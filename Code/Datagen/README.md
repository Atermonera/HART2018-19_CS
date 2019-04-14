This folder contains procedural data generation for testing the kalman filter and frontend. It is capable of producing both .JSON and .CSV formats. clean_data writes the output of the data generator directly to file, while noisy_data adds normal random noise to the measurements with the Box-Mueller method of converting uniform random to normal random distribution.
Simulation for both booster and sustainer are provided within a single call to either, with output file prefixes adjusted accordingly.
The actual data generation is procedural, and so should not differ significantly except in noise applied to the output of noisy_data.

The output files default to a name of `[sst\bst]_[clean/noisy]_[Year]-[Month]-[Day]-[Hours]h-[Minutes]m-[Seconds]s.[JSON/csv]` (Defaulting to csv). The input parameters determine the name and extension of the output files, if it isn't default. All three are optional, but the second and third must come together. The first parameter describes the extension, and must be either `CSV` or `JSON`. The second parameter describes the sustainer output file name. The third parameter describes the booster output file name. Existing files will be overwritten.

Data generation operates according to a schedule of acceleration-durations, specified by the launch_profile array supplied to gen_data(). gen_data() also takes as parameters the timestep between datapoints, and the total duration of simulation. Clean_data and noisy_data both provide 240 seconds of simulation at 100 points per second. The duration was chosenas the minimum to fully encapsulate the flight of the sustainer to minimize output file size, down from an original duration of 600 seconds.
The acceleration is applied as a flat value for the duration, in feet per seconds squared. Vertical motion is derived from this acceleration according to kinematics, with an optional simplified drag calculation according to the current vertical velocity, the current altitude, and a hardcoded "terminal" velocity. 
The terminal velocity is updated when the rocket falls past specific altitudes, simulating parachute deployment. The terminal velocities are taken from personal knowledge of the target velocity of each parachute. The drag is scaled inversely by altitude to simulate the effects of decreasing drag with decreasing atmospheric pressure as a result of rising high into the atmosphere. 
Acceleration values used were approximated from actual simulations of the rocket's flight using older predicted motor burns.
An early attempt was made to properly simulate drag, calculating pressure according to more accurate physical representations, but it resulted in altitudes of 2 astronomical units (Earth-Sun distances) and speeds a great many times in excess the speed of light, within 4 seconds of launch. Such accurate drag simulations were not so important to the general function of the data generator for me to attempt to salvage such inaccurate results.

Gyroscopic axes are represented by a 3x3 array containing a unit vector to describe each axis. These axes should always be orthogonal to one another. Because the rocket will be within its own frame of motion, tracking these axes is important to note both its state (tumbling, spin-stabilized, etc.) and in determining the fixed-axis (x,y,z) accelerations from the variable-axis measurements by the accelerometer, which shares the axes of the gyroscope.
Rotational torque about the vertical axis is applied as a function of vertical velocity and altitude. Moving faster at lower altitudes causes the greatest change in rotation, because the rotation is caused by the rocket's fins, which act as aerofoils.
Rotation about either vertical axis was not implemented, because while I can mentally picture the effect of rapid rotation about a precessing vertical axis, I cannot so easily visualize such an effect in data, and am unsure how to programatically apply the horizontal rotation to properly achieve that. Furthermore, I believe the fins are designed to minimize such precession until the rocket reaches apogee, after which point it will more likely tumble.
Hoizontal translation,however, is implemented. A random direction is chosen for wind, and updated every 1000 feet. A constant, small acceleration is applied in the direction of the wind.

Temperature is calculated according to the International Standard Atmosphere (ISA), which defines fixed deltas between fixed altitudes. Temperature is measured in kelvins, and starts at roughly 40C, which approximates the roughly 100F weather we expect to fly in at Spaceport America in late June.

When the rocket lands, reaching an altitude of <= 0ft, motion is forceably halted.
The discrete nature of the datapoints creates some procedural inaccuracy, notably when the terminal velocity is updated. The changes are very pronounced, from 250 to 80 to 10, which causes very dramatic spikes in vertical acceleration. This could be alleviated by steadily adjusting the terminal velocity over multiple timesteps, but it does also present a particularly good test for the Kalman Filter, which will be expected to ignore a similar data spike in the barometric altitude during actual rocket flight due to the formation of a shockwave as the rocket passes Mach 1.

In all cases, the axes of motion are defined as thus:
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
	
update_alignment.c contains a function to apply 3 rotation matrices to the 3x3 unit-vector notation of the orientation of gyroscopic axes, performing one rotation about each axis.
This is the simplest way I could conceive of perform such a rotation, and I cannot well visualize the actual effects of a rotation about such complex axes as those that aren't aligned with the physical axes of the gyroscope.

To visualize the output, it is recommended that the .csv outputs be loaded into a spreadsheet managing software, such as Excel or Google Sheets. Once the files are loaded there, compile corresponding columns into graphs. Layer the clean data over the noisy data, or it will likely be obscured.
The gyroscopic axes, being represented by unit vectors such as they are, are ill-suited to such a manner of visualization. Visual examination of the data should confirm that they are orthogonal to one another, but I don't have a solution beyond that.
If you know of a tool to display three-dimensional unit vectors as lines within a sphere, that does not require significant setup, I'd recommend using it for the gyroscope axes, and would very much like to know about it myself for further tuning of the filter.