#include <modbus/modbus.h>
#include <pigpio.h>
#include <iostream>
#include <thread>
#include <chrono>

#define RED_LED 23
#define YELLOW_LED 24
#define GREEN_LED 25

modbus_t *ctx;

void on_off(int red_pin, int yellow_pin, int green_pin, bool red_state, bool yellow_state, bool green_state);
void keep_led_on(int red_pin, int yellow_pin, int green_pin, bool red_state, bool yellow_state, bool green_state);

// LED 제어 함수 (모든 LED를 한 번에 제어)
void on_off(int red_pin, int yellow_pin, int green_pin, bool red_state, bool yellow_state, bool green_state) {
    gpioWrite(red_pin, red_state ? 1 : 0);
    gpioWrite(yellow_pin, yellow_state ? 1 : 0);
    gpioWrite(green_pin, green_state ? 1 : 0);

    if (red_state) {
        std::cout << "Red LED is ON." << std::endl;
    } else {
        std::cout << "Red LED is OFF." << std::endl;
    }

    if (yellow_state) {
        std::cout << "Yellow LED is ON." << std::endl;
    } else {
        std::cout << "Yellow LED is OFF." << std::endl;
    }

    if (green_state) {
        std::cout << "Green LED is ON." << std::endl;
    } else {
        std::cout << "Green LED is OFF." << std::endl;
    }
}

// 신호등 제어 함수
void control_traffic_lights(uint16_t reg[100]) {
    bool red_state = false, yellow_state = false, green_state = false;

    // RED LED 상태 결정
    if (reg[0] == 201 && reg[1] == 1) {
        red_state = true;
    } else if (reg[0] == 202 && reg[1] == 0) {
        red_state = false;
    }

    // YELLOW LED 상태 결정
    if (reg[8] == 201 && reg[9] == 1) {
        yellow_state = true;
    } else if (reg[8] == 202 && reg[9] == 0) {
        yellow_state = false;
    }

    // GREEN LED 상태 결정
    if (reg[64] == 301 && reg[65] == 1) {
        green_state = true;
    } else if (reg[64] == 302 && reg[65] == 0) {
        green_state = false;
    }

    keep_led_on(RED_LED, YELLOW_LED, GREEN_LED, red_state, yellow_state, green_state);
}

// LED 상태 유지 함수 (지정된 상태로 유지)
void keep_led_on(int red_pin, int yellow_pin, int green_pin, bool red_state, bool yellow_state, bool green_state) {
    on_off(red_pin, yellow_pin, green_pin, red_state, yellow_state, green_state);
    std::this_thread::sleep_for(std::chrono::seconds(240)); // 지정된 시간 동안 상태 유지
    on_off(red_pin, yellow_pin, green_pin, false, false, false);  // 시간이 지난 후 모든 LED 끔
}

// Modbus 클라이언트 실행 함수
void run_client() {
    const char *port = "/dev/ttyUSB0";  
    int baud = 9600;
    char parity = 'N';
    int data_bit = 8;
    int stop_bit = 1;

    ctx = modbus_new_rtu(port, baud, parity, data_bit, stop_bit);
    if (ctx == nullptr) {
        std::cerr << "Unable to create the libmodbus context." << std::endl;
        return;
    }

    modbus_set_slave(ctx, 1);  

    if (modbus_connect(ctx) == -1) {
        std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return;
    }

    modbus_set_debug(ctx, TRUE);
    std::cout << "Modbus client connected." << std::endl;

    uint16_t reg[100]; 
    modbus_set_response_timeout(ctx, 11, 0);

    while (true) {  // 계속해서 데이터 요청 및 처리
        std::cout << "Requesting data from server..." << std::endl;
        int rc = modbus_read_registers(ctx, 503, 67, reg);
        if (rc == -1) {
            std::cerr << "Read failed: " << modbus_strerror(errno) << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));  // 재시도 대기 시간
        } else {
            control_traffic_lights(reg);
        }
        sleep(600);
    }

    modbus_close(ctx);
    modbus_free(ctx);
    std::cout << "Modbus client stopped." << std::endl;
}

int main() {
    if (gpioInitialise() < 0) {
        std::cerr << "pigpio 초기화 실패!" << std::endl;
        return 1;
    }

    gpioSetMode(RED_LED, PI_OUTPUT);
    gpioSetMode(YELLOW_LED, PI_OUTPUT);
    gpioSetMode(GREEN_LED, PI_OUTPUT);

    std::thread modbus_thread(run_client);

    modbus_thread.join();  // Modbus 스레드가 끝날 때까지 대기
    gpioTerminate();  // GPIO 리소스 해제

    return 0;
}
