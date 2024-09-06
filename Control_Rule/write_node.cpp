#include <modbus/modbus.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <cerrno>
#include <cstdint>

// 전역 변수 선언
modbus_t *ctx;  // 여기에 선언

modbus_mapping_t *mb_mapping;  // 추가: Modbus 매핑을 위한 전역 변수 선언

void run_server() {
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
    mb_mapping->tab_registers[1] = 0; // 기관코드
    mb_mapping->tab_registers[2] = 0; // 회사코드 
    mb_mapping->tab_registers[3] = 2; // 제품타입(구동기노드 : 2)
    mb_mapping->tab_registers[4] = 0; // 제품코드
    mb_mapping->tab_registers[5] = 10; // 프로토콜버전
    mb_mapping->tab_registers[6] = 24; // 채널
    mb_mapping->tab_registers[7] = 0; // 노드 시리얼 번호(unit 32) 
    mb_mapping->tab_registers[8] = 0; // 노드 시리얼 번호(unit 32)
    // 9 ~ 100은 예비용(추후 확장 가능)
    
    for (int i = 101; i <= 116; ++i) { // 사용 않는 것을 0x00으로 수정해야
		mb_mapping->tab_registers[i] = 102; // 스위치형 구동기 디바이스 코드(102)
	}
	
	for (int i = 117; i <= 124; ++i) {
		mb_mapping->tab_registers[i] = 112; // 개폐형 구동기 디바이스 코드(112)
	}
    함
    mb_mapping->tab_registers[201] = 2.0; // OPID #0 ??
    mb_mapping->tab_registers[202] = 2.0; // 구동기 노드 상태 ??
    
    mb_mapping->tab_registers[501] = 2.0; // 노드 명령 ??
    mb_mapping->tab_registers[502] = 2.0; // OPID #0 ??
    
    // 구동기 상태 정보
    for (int i = 203; i <= 298; ) {
		mb_mapping->tab_registers[i++] = 0x0000; // OPID; 16비트 0으로 초기화
		mb_mapping->tab_registers[i++] = 0x0000; // 스위치 상태; 16비트 0으로 초기화
		
		unit32_t value_32bit = 0x00000000; //스위치 남은동작시간; 32비트 0으로 초기화
		mb_mapping->tab_registers[i++] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연
		mb_mapping->tab_registers[i++] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산
	}
	
	///////////////////////////////////////////////구동기 상태 정보////////////////////////////////////////////////
	
	// 스위치 1(냉방기)에 대한 상태 정보
	mb_mapping->tab_registers[203] = 2.1; // OPID #1
	mb_mapping->tab_registers[204] = 2.0; // 스위치 1 상태
	mb_mapping->tab_registers[205] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(남은 동작 시간)
	mb_mapping->tab_registers[206] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(남은 동작 시간)
	
	// 스위치 2(난방기)에 대한 상태 정보
	mb_mapping->tab_registers[207] = 2.2; // OPID #2
	mb_mapping->tab_registers[208] = 2.0; // 스위치 2 상태
	mb_mapping->tab_registers[209] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(남은 동작 시간)
	mb_mapping->tab_registers[210] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(남은 동작 시간)
	
	// 스위치 3(환풍기)에 대한 상태 정보
	mb_mapping->tab_registers[211] = 2.3; // OPID #3
	mb_mapping->tab_registers[212] = 2.0; // 스위치 3 상태 
	mb_mapping->tab_registers[213] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(남은 동작 시간)
	mb_mapping->tab_registers[214] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(남은 동작 시간)
	
	// 개폐기 1(창문)에 대한 상태 정보 
	mb_mapping->tab_registers[267] = 2.4; // OPID #17
	mb_mapping->tab_registers[268] = 2.0; // 개폐기 1 상태
	mb_mapping->tab_registers[269] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(남은 동작 시간)
	mb_mapping->tab_registers[270] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(남은 동작 시간)
	
	///////////////////////////////////////////////구동기 제어 정보////////////////////////////////////////////////
	
    // 스위치 1(냉방기)에 대한 상태 정보
	mb_mapping->tab_registers[503] = 2.1; // 스위치 1 명령
	mb_mapping->tab_registers[504] = 2.1; // OPID #1
	mb_mapping->tab_registers[505] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(동작시간)
	mb_mapping->tab_registers[506] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(동작시간)
	
	// 스위치 2(난방기)에 대한 상태 정보
	mb_mapping->tab_registers[507] = 2.2; // 스위치 2 명령
	mb_mapping->tab_registers[508] = 2.2; // OPID #2
	mb_mapping->tab_registers[509] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(동작시간)
	mb_mapping->tab_registers[510] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(동작시간)
	
	// 스위치 3(환풍기)에 대한 상태 정보
	mb_mapping->tab_registers[511] = 2.3; // 스위치 3 명령
	mb_mapping->tab_registers[512] = 2.3; // OPID #3 
	mb_mapping->tab_registers[513] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(동작시간)
	mb_mapping->tab_registers[514] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(동작시간)
	
	// 개폐기 1(창문)에 대한 상태 정보 
	mb_mapping->tab_registers[567] = 2.4; // 개폐기 1 명령
	mb_mapping->tab_registers[568] = 2.4; // OPID #17
	mb_mapping->tab_registers[569] = value_32bit & 0xFFFF; // 하위 16비트를 추출하기 위한 연산(동작시간)
	mb_mapping->tab_registers[570] = (value_32bit >> 16) & 0xFFFF; // 상위 16비트를 추출하기 위한 연산(동작시간)
	
    while (true) {
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
            continue;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 자원 해제
    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);
    std::cout << "Modbus server stopped." << std::endl;
}

int main() {
    run_server();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}
