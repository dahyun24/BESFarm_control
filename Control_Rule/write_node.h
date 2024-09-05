#ifndef WRITE_NODE_H
#define WRITE_NODE_H

#include <string>
#include <ctime>  
void writeNode(bool isFanOn, bool isWindowOpen, bool isCoolingOn, bool isHeatingOn, std::time_t now);

#endif