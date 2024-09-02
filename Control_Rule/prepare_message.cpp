#include <iostream>
#include <ctime>
#include <string>
#include "prepare_message.h"

std::string prepareMessage(bool isFanOn, bool isWindowOpen, bool isCoolingOn, bool isHeatingOn, std::time_t now) {
    std::tm *ltm = std::localtime(&now);
    char timeBuffer[100]; // 충분한 크기의 버퍼

    // 시간을 문자열로 포맷합니다.
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", ltm);

    std::string message = "Time: " + std::string(timeBuffer) + "\n";

    message += "Fan: " + std::string(isFanOn ? "ON" : "OFF") + "\n";
    message += "Window: " + std::string(isWindowOpen ? "OPEN" : "CLOSE") + "\n";
    message += "Cooling: " + std::string(isCoolingOn ? "ON" : "OFF") + "\n";
    message += "Heating: " + std::string(isHeatingOn ? "ON" : "OFF") + "\n";

    return message;
}
