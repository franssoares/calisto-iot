#pragma once
#include <Arduino.h>

void setupSensors();
float getTemperature();
void getVibrationRMS(float &rmsX, float &rmsY, float &rmsZ);