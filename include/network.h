#pragma once
#include <Arduino.h>

void setupNetwork();
void keepNetworkAlive();
void publishData(float temp, float rmsX, float rmsY, float rmsZ);