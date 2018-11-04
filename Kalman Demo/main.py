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
generate # Run generate.py to create curves for barometric pressure and velocity

velocityFile = open("smoothVelocity", "w")
velocityFile.write("Time (s), Velocity\n")
pressureFile = open("smoothPressure", "w")
pressureFile.write("Time (s), Pressure\n")


pressureData = open("pressure", "r")
velocityData = open("velocity", "r")
accelerationData = open("acceleration", "r")

#Get header files out of the way
pressureLine = pressureData.readline()
velocityLine = velocityData.readline()
accelerationLine = accelerationData.readline()
#Read first data lines
pressureLine = pressureData.readline()
velocityLine = velocityData.readline()
accelerationLine = accelerationData.readline()
timeV, velocity = map(float, velocityLine.split(','))
timeP, pressure = map(float, pressureLine.split(','))

velocityFilter = kalmanFilter.velocityKalman(pressure, pressure * pressureUncertainty * 0.5, velocity, 5, 5)

# def calcAccel(x):
#     return -2*(x - 500)/625 # Derivative of our velocity equation, this finds the acceleration given time for the kalman filter

while(velocityLine):
    timeV, velocity = map(float, velocityLine.split(','))
    velocityLine = pressureData.readline()
    timeP, pressure = map(float, pressureLine.split(','))
    pressureLine = velocityData.readline()
    timeA, accel = map(float, accelerationLine.split(','))
    accelerationLine = accelerationData.readline()
    velocityFilter.newMeasurement(pressure, pressureUncertainty * pressure, velocity, velocity * velocityUncertainty, accel)
    velocityFile.write("%f, %f+-%f\n" % (timeV, velocityFilter.getCurrentVelocity(), velocityFilter.getCurrentVelocityUncertainty()))
    pressureFile.write("%f, %f+-%f\n" % (timeV, velocityFilter.getCurrentPressure(), velocityFilter.getCurrentPressureUncertainty()))

velocityFile.close()
pressureData.close()
velocityData.close()