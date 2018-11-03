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

velocityFilter = kalmanFilter.velocityKalman(initialVelocity, 5, 5)

velocityFile = open("smoothVelocity", "w")
velocityFile.write("Time (s), Velocity\n")

pressureData = open("pressure", "r")
velocityData = open("velocity", "r")

#Get header files out of the way
pressureLine = pressureData.readline()
velocityLine = velocityData.readline()
#Read first data lines
pressureLine = pressureData.readline()
velocityLine = velocityData.readline()

while(velocityLine):
    timeV, velocity = map(float, pressureLine.split(','))
    pressureLine = pressureData.readline()
    timeP, pressure = map(float, velocityLine.split(','))
    velocityLine = velocityData.readline()

velocityFile.close()

def calcAccel(x):
    return -2*(x - 500)/625 # Derivative of our velocity equation, this finds the acceleration given time for the kalman filter