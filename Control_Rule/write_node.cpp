#include <iostream>
#include <string>
#include <ctime>
#include <modbus/modbus.h>
#include <cstring>
#include <cerrno>
#include <thread>
#include "write_node.h"

// 전역 변수
modbus_t *ctx = nullptr;
modbus_mapping_t *mb_mapping = nullptr;
uint32_t value_32bit = 0x00000000; //32비트 0으로 초기화

// 레지스터 맵을 생성하고 값을 설정
void writeNode(bool isFanOn, bool isWindowOpen, bool isCoolingOn, bool isHeatingOn, std::time_t now) {
    if (ctx == nullptr) {
        const char *port = "/dev/ttyUSB0";  // 시리얼 포트 설정
        int baud = 9600;
        char parity = 'N';
        int data_bit = 8;
        int stop_bit = 1;

        ctx = modbus_new_rtu(port, baud, parity, data_bit, stop_bit);
        if (ctx == nullptr) {
            std::cerr << "Unable to create the libmodbus context." << std::endl;
            return;
        }

        std::cout << "Modbus context created." << std::endl;
        modbus_set_slave(ctx, 1);

        if (modbus_connect(ctx) == -1) {
            std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
            modbus_free(ctx);
            return;
        }

        modbus_set_debug(ctx, TRUE);
        std::cout << "Modbus server started." << std::endl;
    }
    // Modbus 매핑 설정 (레지스터 20개)
    if (mb_mapping == nullptr) {
        mb_mapping = modbus_mapping_new(0, 0, 570, 0);
        if (mb_mapping == nullptr) {
            std::cerr << "Failed to allocate the mapping: " << modbus_strerror(errno) << std::endl;
            modbus_close(ctx);
            modbus_free(ctx);
            return;
        }
    }
	
    ///////////////////////////////////////////////구동기 제어 정보////////////////////////////////////////////////

    // 스위치 1(냉방기)에 대한 상태 정보
    if (isCoolingOn) {
        mb_mapping->tab_registers[503] = 201; // 냉방기 스위치 ON
        mb_mapping->tab_registers[504] = 1; // OPID #1 - ON
    } else {
        mb_mapping->tab_registers[503] = 202; // 냉방기 스위치 OFF
        mb_mapping->tab_registers[504] = 0; // OPID #0 - OFF
    }
    mb_mapping->tab_registers[505] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(동작시간)
    mb_mapping->tab_registers[506] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(동작시간)

    // 스위치 2(난방기)에 대한 상태 정보
    if (isHeatingOn) {
        mb_mapping->tab_registers[507] = 201; // 난방기 스위치 ON
        mb_mapping->tab_registers[508] = 1; // OPID #1 - ON
    } else {
        mb_mapping->tab_registers[507] = 202; // 난방기 스위치 OFF
        mb_mapping->tab_registers[508] = 0; // OPID #0 - OFF
    }
    mb_mapping->tab_registers[509] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(동작시간)
    mb_mapping->tab_registers[510] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(동작시간)

    // 스위치 3(환풍기)에 대한 상태 정보
    if (isFanOn) {
        mb_mapping->tab_registers[511] = 201; // 환풍기 스위치 ON
        mb_mapping->tab_registers[512] = 1; // OPID #1 - ON
    } else {
        mb_mapping->tab_registers[511] = 202; // 환풍기 스위치 OFF
        mb_mapping->tab_registers[512] = 0; // OPID #0 - OFF
    }
    mb_mapping->tab_registers[513] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(동작시간)
    mb_mapping->tab_registers[514] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(동작시간)

    // 개폐기 1(창문)에 대한 상태 정보
    if (isWindowOpen) {
        mb_mapping->tab_registers[567] = 301; // 창문 열림 명령
        mb_mapping->tab_registers[568] = 1; // OPID #1 - ON
    } else {
        mb_mapping->tab_registers[567] = 302; // 창문 닫힘 명령
        mb_mapping->tab_registers[568] = 0; // OPID #0 - OFF
    }
    mb_mapping->tab_registers[569] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(동작시간)
    mb_mapping->tab_registers[570] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(동작시간)

    uint8_t query[MODBUS_RTU_MAX_ADU_LENGTH];
    memset(query, 0, sizeof(query));
    std::cout << "Waiting for an indication..." << std::endl;
    int rc = modbus_receive(ctx, query);
        
    if (rc > 0) {
        std::cout << "Received request from client, processing..." << std::endl;
        int reply_rc = modbus_reply(ctx, query, rc, mb_mapping);
        if (reply_rc == -1) {
            std::cerr << "Error sending response: " << modbus_strerror(errno) << std::endl;
        } else {
            std::cout << "Response sent to client." << std::endl;
        }
    } else if (rc == -1) {
        std::cerr << "Error receiving request: " << modbus_strerror(errno) << std::endl;
        modbus_flush(ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

    // 자원 해제
void cleanUpModbus() {
    if (mb_mapping) {
        modbus_mapping_free(mb_mapping);
        mb_mapping = nullptr;
    }
    if (ctx) {
        modbus_close(ctx);
        modbus_free(ctx);
        ctx = nullptr;
    }
}
