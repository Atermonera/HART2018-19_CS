#!/usr/bin/python
import math
import random

#Constants
startTime = 0.0
frequency = 5.0 #Frequency, in Hz, to update the velocity

#Math Variables
currentTime = startTime # Current time, in seconds
velocity = 0.0
velocityUncertainty = 0.05 # Assume 5% base uncertainty for the velocity
pressure = 0.0
pressureUncertainty = 0.1 # Assume 10% base uncertainty for the barometric pressure
random.seed()
zeroPressureStart, zeroPressureEnd = 500, 505 # Times to zero out pressure, for testing shock waves
pressureUncertaintyIndex = 180 # Pressure index to increase uncertainty, to simulate high altitude
maxVelocity = 400

with open("velocity", "w") as file:
    file.write("Time (s), Velocity\n")
    while (currentTime <= 1000): #Update the velocity for 1000 seconds
        velocity = -0.0016 * math.pow(currentTime - 500, 2) + 400.0 # Quadratic equation to simulate velocity
  
        if velocity > maxVelocity * .7: #Simulate CAPCOM restriction on gps data over certain velocity
            velocity = 0
    
        #Create uncertainty in velocity
        randVariance = random.uniform(velocity * velocityUncertainty, -1 * velocity * velocityUncertainty)
        velocity += randVariance
        
        file.write("%f, %f\n" % (currentTime, velocity))
        currentTime += 1.0/frequency

    currentTime = 0
    velocity = 0

with open("pressure", "w") as file:
    file.write("Time (s), Pressure\n")
    while (currentTime <= 1000): #Update the pressure for 1000 seconds
        if currentTime >= zeroPressureStart and currentTime <= zeroPressureEnd: #Simulate 5 second error where pressure reads 0
            pressure = 0
        else:
            pressure = -0.0005 * math.pow(currentTime - 500, 2) + 200 # Quadratic equation to simulate pressure
        
        #Create uncertainty in pressure...
        randVariance = random.uniform(pressure * pressureUncertainty, -1 * pressure * pressureUncertainty)
        if pressure > pressureUncertaintyIndex: #Simulate higher variance at high alititudes
            randVariance *= 2
        pressure += randVariance

        file.write("%f, %f\n" % (currentTime, pressure))
        currentTime += 1.0/frequency
    currentTime = 0
    pressure = 0