import kalmanFilter
import generate

#Math Variables
initialVelocity = 0
initialPressure = 75
velocityUncertainty = 0.05 #Assume 5% base uncertainty for the velocity
pressureUncertainty = 0.2 #Assume 10% base uncertainty for the barometric pressure
zeroPressureStart, zeroPressureEnd = 500, 505 # Times to zero out pressure, for testing shock waves
pressureUncertaintyIndex = 180 # Pressure index to increase uncertainty, to simulate high altitude
maxVelocity = 400

#Start running things
generate

velocityFilter = kalmanFilter.kalman(initialVelocity, 100)
pressureFilter = kalmanFilter.kalman(initialPressure, initialPressure * pressureUncertainty)

velocityFile = open("smoothVelocity", "w")
velocityFile.write("Time (s), Velocity\n")
pressureFile = open("smoothPressure", "w")
pressureFile.write("Time (s), Pressure\n")

with open("velocity", "r") as file:
    file.readline() # Get rid of header line
    line = file.readline()
    while(line):
        time, velocity = map(float, line.split(','))
        velocityFilter.newMeasurement(velocity, velocity * velocityUncertainty)
        velocityFile.write("%f, %f\n" % (time, velocityFilter.getCurrentEstimate()))
        line = file.readline()

with open("pressure", "r") as file:
    file.readline() # Get rid of header line
    line = file.readline()
    while(line):
        time, pressure = map(float, line.split(','))
        pressureFilter.newMeasurement(pressure, pressure * pressureUncertainty)
        pressureFile.write("%f, %f\n" % (time, pressureFilter.getCurrentEstimate()))
        line = file.readline()

velocityFile.close()
pressureFile.close()