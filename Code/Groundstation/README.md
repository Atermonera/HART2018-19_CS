receiver.c contains the framework to eventually receive live telemetry from the rocket, pass it through the kalman filter, and then send it to the front end.
serial_telemetry.c contains code from the ECE subteam to handle the incoming packets and parse them into useful variables. They've tested their functions, though I've been rather busier working on the kalman filter to do so myself.
The structure of receiver.c should be roughly similar to what they used to test the USB interface, but I don't have anything to supply appropriate data to the pipe.

This code compiles, but it doesn't actually necessarily do anything. I haven't had time to read through serial_telemetry.c to figure out how to format data to send into it.