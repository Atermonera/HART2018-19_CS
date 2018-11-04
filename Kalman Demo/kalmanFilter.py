#!/usr/bin/python
import math
import numpy as np
class velocityKalman:

    def __init__(self, initialPressure, initialPressureUncertainty, initialVelocity, initialVelocityUncertainty, frequency):
        self.currentState_mat = np.matrix([[initialPressure], [initialVelocity]])
        self.A_mat = np.matrix([[1, 1.0/frequency], [0, 1]])
        self.B_mat = np.matrix([0.5 * math.pow(1.0/frequency, 2), 1.0/frequency])
        self.H_mat = np.matrix([[1, 0], [0, 1]])
        self.I_mat = np.matrix([[1, 0], [0, 1]])
        self.pc_mat = np.matrix([[math.pow(initialPressureUncertainty, 2), 0],[0, math.pow(initialVelocityUncertainty, 2)]])
    
    #Pass the filter a new measurement, which it uses to update it's current estimate
    def newMeasurement(self, pressure, pressureUncertainty, velocity, velocityUncertainty, acceleration):
        

        #Calculate the predicted state
        predictedState_mat = self.A_mat * self.currentState_mat + self.B_mat * acceleration

        #Calculate the predicted process covariance matrix
        pcpredict_mat = self.A_mat * self.pc_mat * self.A_mat.transpose()
        pcpredict_mat.flat[1] = 0
        pcpredict_mat.flat[2] = 0

        #Calculate the Kalman Gain
        R_mat = np.matrix([[pressureUncertainty**2, 0], [0, velocityUncertainty**2]])

        print("PCP: \n", pcpredict_mat)
        print("R_mat: \n", R_mat)
        kalmanGain_mat = pcpredict_mat * self.H_mat.transpose() / (self.H_mat * pcpredict_mat * self.H_mat.transpose() + R_mat)
        print("K_mat: \n", kalmanGain_mat)

        #Format new observation
        Y_mat = np.matrix([pressure, velocity])

        #Calculate the current state
        self.currentState_mat = predictedState_mat + kalmanGain_mat * (Y_mat - self.H_mat * predictedState_mat)

        #Update covariance matrix
        self.pc_mat = (self.I_mat - kalmanGain_mat * self.H_mat) * pcpredict_mat

    def getCurrentVelocity(self):
        return self.currentState_mat.flat[0]

    def getCurrentVelocityUncertainty(self):
        return self.pc_mat