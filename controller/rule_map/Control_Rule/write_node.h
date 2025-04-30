#ifndef WRITE_NODE_H
#define WRITE_NODE_H

#include <string>
#include <ctime>
#include <modbus/modbus.h> 

void writeNode(bool isFanOn, bool isWindowOpen, bool isCoolingOn, bool isHeatingOn, std::time_t now);
void cleanUpModbus();

#endif