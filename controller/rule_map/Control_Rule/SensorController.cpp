#include "SensorController.h"
#include "write_node.h"
#include <iostream>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>  // For std::memcpy
#include <cstdint> // For uint8_t


// 생성자: 객체 초기화
SensorController::SensorController(const char* server, const char* user, const char* password, const char* database)
    : isFanOn(false), isWindowOpen(false), isCoolingOn(false), isHeatingOn(false), portName("/dev/ttyUSB1") {
    connectToDatabase(server, user, password, database);
}

// 소멸자: 리소스 해제
SensorController::~SensorController() {
    disconnectFromDatabase();
}

// 데이터베이스에 연결
void SensorController::connectToDatabase(const char* server, const char* user, const char* password, const char* database) {
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        std::cerr << "MySQL connection failed: " << mysql_error(conn) << std::endl;
        exit(1);
    }
}

// 데이터베이스 연결 종료
void SensorController::disconnectFromDatabase() {
    if (conn) {
        mysql_close(conn);
    }
}

// 바이트 배열을 float로 변환
float SensorController::bytesToFloat(uint8_t* bytes) {
    // Little-endian 방식으로 바이트 배열을 float으로 변환
    uint32_t value = (static_cast<uint32_t>(bytes[0]) << 24) |
        (static_cast<uint32_t>(bytes[1]) << 16) |
        (static_cast<uint32_t>(bytes[2]) << 8) |
        (static_cast<uint32_t>(bytes[3]));

    float result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

// 센서 데이터를 데이터베이스에 삽입
void SensorController::insertDataToDB(float temperature, float humidity, float co2Concentration) {
    time_t now = time(0);
    char datetime[20];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime(&now));

    char query[256];
    snprintf(query, sizeof(query),
             "INSERT INTO sensor (temp, humidity, co2, date) VALUES (%.2f, %.2f, %.2f, '%s')",
             temperature, humidity, co2Concentration, datetime);

    if (mysql_query(conn, query)) {
        std::cerr << "INSERT failed. Error: " << mysql_error(conn) << std::endl;
    } else {
		std::cout << "Date: " << datetime << std::endl;
        std::cout << "Data inserted successfully" << std::endl;
    }
}

// 제어 상태를 데이터베이스에 동기화
void SensorController::syncControlStateToDB() {
    std::string fanState = isFanOn ? "on" : "off";
    std::string windowState = isWindowOpen ? "open" : "close";
    std::string coolingState = isCoolingOn ? "on" : "off";
    std::string heatingState = isHeatingOn ? "on" : "off";

    std::string query = "INSERT INTO control_rule (fan, window, cooling, heating, date) VALUES ('" +
                        fanState + "', '" +
                        windowState + "', '" +
                        coolingState + "', '" +
                        heatingState + "', NOW())";

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Sync INSERT query failed: " << mysql_error(conn) << std::endl;
    } else {
        std::cout << "Control state synced to the database." << std::endl;
        std::cout << "Saved values - Fan: " << fanState
                  << ", Window: " << windowState
                  << ", Cooling: " << coolingState
                  << ", Heating: " << heatingState << std::endl;
    }
}

// 센서 데이터를 기반으로 제어 수행
void SensorController::checkAndControl(float temp, float humidity, int co2) {
    if (co2 >= 1000 && !isFanOn) {
        std::cout << "환풍기 ON" << std::endl;
        isFanOn = true;
    } else if (co2 <= 800 && isFanOn) {
        std::cout << "환풍기 OFF" << std::endl;
        isFanOn = false;
    } else {
        std::cout << "환풍기 상태 유지" << std::endl;
    }

    if (humidity >= 70 && !isWindowOpen) {
        std::cout << "천창 OPEN" << std::endl;
        isWindowOpen = true;
    } else if (humidity <= 50 && isWindowOpen) {
        std::cout << "천창 CLOSE" << std::endl;
        isWindowOpen = false;
    } else {
        std::cout << "천창 상태 유지" << std::endl;
    }

    if (temp >= 28 && !isCoolingOn) {
        std::cout << "냉방기 ON" << std::endl;
        isCoolingOn = true;
        isHeatingOn = false;
    } else if (temp <= 20 && !isHeatingOn) {
        std::cout << "난방기 ON" << std::endl;
        isHeatingOn = true;
        isCoolingOn = false;
    } else if (temp >= 22 && temp <= 26 && (isCoolingOn || isHeatingOn)) {
        std::cout << "냉난방기 OFF" << std::endl;
        isCoolingOn = false;
        isHeatingOn = false;
    } else {
        std::cout << "냉난방기 상태 유지" << std::endl;
    }
    std::time_t now = std::time(nullptr);
    writeNode(isFanOn, isWindowOpen, isCoolingOn, isHeatingOn, now);
}

// 센서 데이터를 읽고 제어 수행
void SensorController::readSensorDataAndControl() {
    int serialPort = open(portName, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serialPort == -1) {
        std::cerr << "Error opening serial port" << std::endl;
        return;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(serialPort, &tty) != 0) {
        std::cerr << "Error getting serial port attributes" << std::endl;
        close(serialPort);
        return;
    }

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(serialPort, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting serial port attributes" << std::endl;
        close(serialPort);
        return;
    }

    unsigned char request[] = { 0x01, 0x03, 0x00, 0xCA, 0x00, 0x28, 0x65, 0xEA };

    ssize_t bytesWritten = write(serialPort, request, sizeof(request));
    if (bytesWritten != sizeof(request)) {
        std::cerr << "Error writing to serial port" << std::endl;
        close(serialPort);
        return;
    }

    usleep(1000000);

    unsigned char response[256];
    ssize_t totalBytesRead = 0;
    ssize_t bytesRead;

    while (true) {
        bytesRead = read(serialPort, response + totalBytesRead, sizeof(response) - totalBytesRead);
        if (bytesRead > 0) {
            totalBytesRead += bytesRead;
            if (totalBytesRead >= sizeof(response)) {
                break;
            }
        } else {
            break;
        }
    }

    if (totalBytesRead >= 82 && sizeof(response) >= 82) {
        float temperature = bytesToFloat(response + 7);
        float humidity = bytesToFloat(response + 25);
        float co2Concentration = bytesToFloat(response + 79);

		std::cout << "Received data from sensor: " << std::endl;
        std::cout << "Temperature: " << temperature << " °C" << std::endl;
        std::cout << "Humidity: " << humidity << " %" << std::endl;
        std::cout << "CO2 Concentration: " << co2Concentration << " ppm" << std::endl;
     
        insertDataToDB(temperature, humidity, co2Concentration); // 센서 데이터를 DB에 저장
        checkAndControl(temperature, humidity, co2Concentration); // 바로 제어 로직 실행
        syncControlStateToDB(); // 제어 상태를 DB에 저장
    } else {
        std::cerr << "Insufficient data received" << std::endl;
    }

    close(serialPort);
}


// 센서 데이터를 읽고 제어 로직을 수행하는 루프 실행
void SensorController::run() {
    while (true) {
        readSensorDataAndControl(); // 센서 데이터를 읽고 제어 로직을 수행

        std::cout << "Waiting for 20 minutes before next cycle..." << std::endl;
        sleep(1200);  // 20분 대기
    }
}