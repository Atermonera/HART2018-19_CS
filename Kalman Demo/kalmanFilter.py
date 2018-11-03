#!/usr/bin/python
import math
import numpy as np
class velocityKalman:
    # currentState_mat
    # A_mat
    # B_mat
    # H_mat
    # pc_mat
    # kalmanGain_mat
    # R_mat

    def __init__(self, initialPressure, initialPressureUncertainty, initialVelocity, initialVelocityUncertainty, frequency):
        self.currentState_mat = np.matrix([initialPressure, initialVelocity])
        self.A_mat = np.matrix([[1, 1.0/frequency], [0, 1]])
        self.B_mat = np.matrix([0.5 * math.pow(1.0/frequency, 2), 1.0/frequency])
        self.H_mat = np.matrix([[1, 0], [0, 1]])
        self.pc_mat = np.matrix([[math.pow(initialPressureUncertainty, 2), 0],[0, math.pow(initialVelocityUncertainty, 2)]])
        self.pc_
    
    #Pass the filter a new measurement, which it uses to update it's current estimate
    def newMeasurement(self, pressure, pressureUncertainty, velocity, velocityUncertainty, acceleration):
        print "nothing"

    def getCurrentEstimate(self):
        return self.currentState_mat

    def getCurrentUncertainty(self):
        return self.pc_mat