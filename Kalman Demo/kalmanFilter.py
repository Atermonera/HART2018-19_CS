#!/usr/bin/python

class kalman:
    currentEstimate = 0 # This filter's current estimation for the real measurement
    currentUncertainty = 0 # This filter's current estimation of the uncertainty of it's estimation

    def __init__(self, initialEstimate, initialUncertainty):
        self.currentEstimate = initialEstimate
        self.currentUncertainty = initialUncertainty
    
    #Pass the filter a new measurement, which it uses to update it's current estimate
    def newMeasurement(self, measurement, measurementUncertainty):
        kalmanGain = self.currentUncertainty / (self.currentUncertainty + measurementUncertainty)
        self.currentEstimate = self.currentEstimate + kalmanGain * (measurement - self.currentEstimate)
        self.currentUncertainty = (1 - kalmanGain) * self.currentUncertainty

    def getCurrentEstimate(self):
        return self.currentEstimate