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

// 레지스터 맵을 생성하고 값을 설정
void writeNode(bool isFanOn, bool isWindowOpen, bool isCoolingOn, bool isHeatingOn, std::time_t now) {
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

    // Modbus 매핑 설정 (레지스터 20개)
    mb_mapping = modbus_mapping_new(0, 0, 505, 0);
    if (mb_mapping == nullptr) {
        std::cerr << "Failed to allocate the mapping: " << modbus_strerror(errno) << std::endl;
        modbus_close(ctx);
        modbus_free(ctx);
        return;
    }
    mb_mapping->tab_registers[503] = 0;
    mb_mapping->tab_registers[504] = 1; // 504번 주소(인덱스 4) 설정

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
    

    // 자원 해제
    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);
    std::cout << "Modbus server stopped." << std::endl;
}