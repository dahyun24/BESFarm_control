#ifndef PREPARE_MESSAGE_H
#define PREPARE_MESSAGE_H

#include <string>
#include <ctime>

std::string prepareMessage(bool isFanOn, bool isWindowOpen, bool isCoolingOn, bool isHeatingOn, std::time_t now);

#endif
