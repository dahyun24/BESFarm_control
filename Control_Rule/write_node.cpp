#include <iostream>
#include <string>
#include <ctime>
#include "write_node.h"

// 레지스터 맵을 생성하고 값을 설정
void writeNode(bool isFanOn, bool isWindowOpen, bool isCoolingOn, bool isHeatingOn, std::time_t now) {

    std::tm *ltm = std::localtime(&now);
    char timeBuffer[100]; // 충분한 크기의 버퍼

    // 시간을 문자열로 포맷합니다.
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", ltm);

    // 메시지 출력 (추후 실제 통신 구현 시 여기에 추가)

}