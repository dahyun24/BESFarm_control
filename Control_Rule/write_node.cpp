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

    std::string message = "Time: " + std::string(timeBuffer) + "\n";

    // 메시지 출력 (추후 실제 통신 구현 시 여기에 추가)
    std::cout << "Message received by writeNode: \n" << message << std::endl;

    // 메시지의 각 줄을 파싱하여 레지스터 맵에 할당할 수 있음
    // 현재는 단순히 메시지를 표시만 함
    
    // 예시: regMap에 값 할당
    // 실제로는 메시지에서 값들을 파싱하여 아래와 같은 방식으로 할당
    // regMap.fanStatus = (message.find("Fan: ON") != std::string::npos);
    // regMap.windowStatus = (message.find("Window: OPEN") != std::string::npos);
    // regMap.coolingStatus = (message.find("Cooling: ON") != std::string::npos);
    // regMap.heatingStatus = (message.find("Heating: ON") != std::string::npos);

    // regMap.timestamp = 추출된 타임스탬프 값;

    // 여기에 구동기 노드로 통신하는 로직 추가
    // 예를 들어, 네트워크 또는 시리얼 통신을 통해 레지스터 값을 전송
}
