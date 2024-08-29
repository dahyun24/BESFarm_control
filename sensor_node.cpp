#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include <iomanip> // For std::setw and std::setfill
#include <cstring> // For std::memcpy
#include <cstdint> // For uint8_t
#include <mariadb/mysql.h> // MariaDB/MySQL C API

float bytesToFloat(uint8_t* bytes) {
    // Little-endian 방식으로 바이트 배열을 float으로 변환
    uint32_t value = (static_cast<uint32_t>(bytes[0]) << 24) |
        (static_cast<uint32_t>(bytes[1]) << 16) |
        (static_cast<uint32_t>(bytes[2]) << 8) |
        (static_cast<uint32_t>(bytes[3]));

    float result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

void insertDataToDB(float temperature, float humidity, float co2Concentration) {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    const char* server = "192.168.0.68"; // MySQL 서버 주소
    const char* user = "besf"; // MySQL 사용자 이름
    const char* password = "besf0905"; // MySQL 비밀번호
    const char* database = "sensor_data"; // 데이터베이스 이름

    conn = mysql_init(NULL);

    if (conn == NULL) {
        std::cerr << "mysql_init() failed\n";
        return;
    }

    if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0) == NULL) {
        std::cerr << "mysql_real_connect() failed\n";
        mysql_close(conn);
        return;
    }

    // 현재 시간을 DATETIME 형식으로 얻기
    time_t now = time(0);
    char datetime[20];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // SQL 쿼리 준비
    char query[256];
    snprintf(query, sizeof(query),
             "INSERT INTO sensor (temp, humidity, co2, date) VALUES (%.2f, %.2f, %.2f, '%s')",
             temperature, humidity, co2Concentration, datetime);

    // SQL 쿼리 실행
    if (mysql_query(conn, query)) {
        std::cerr << "INSERT failed. Error: " << mysql_error(conn) << std::endl;
    } else {
        std::cout << "Data inserted successfully" << std::endl;
    }

    mysql_close(conn);
}

int main() {
    const char* portName = "/dev/ttyUSB1"; // 라즈베리 파이의 기본 UART 포트 경로 (확인 필요)

    // 시리얼 포트 열기
    int serialPort = open(portName, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serialPort == -1) {
        std::cerr << "Error opening serial port" << std::endl;
        return 1;
    }

    // 시리얼 포트 설정
    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(serialPort, &tty) != 0) {
        std::cerr << "Error getting serial port attributes" << std::endl;
        close(serialPort);
        return 1;
    }

    // Baud rate 설정
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    // 8N1 모드 설정: 8비트 데이터, No parity, 1 stop bit
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK; // ignore break signal
    tty.c_lflag = 0; // no signaling chars, no echo,
    tty.c_oflag = 0; // no remapping, no delays
    tty.c_cc[VMIN]  = 0; // read doesn't block
    tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout

    tty.c_cflag |= (CLOCAL | CREAD); // turn on READ & ignore ctrl lines
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag &= ~CSTOPB; // 1 stop bit
    tty.c_cflag &= ~CRTSCTS; // no hardware flow control

    if (tcsetattr(serialPort, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting serial port attributes" << std::endl;
        close(serialPort);
        return 1;
    }

    unsigned char request[] = { 0x01, 0x03, 0x00, 0xCA, 0x00, 0x28, 0x65, 0xEA };

    while (true) { // 무한 루프를 통해 5분마다 데이터를 받아옴
        // 데이터 전송
        ssize_t bytesWritten = write(serialPort, request, sizeof(request));
        if (bytesWritten != sizeof(request)) {
            std::cerr << "Error writing to serial port" << std::endl;
            close(serialPort);
            return 1;
        }

        usleep(1000000);  // 1초 대기 (1000000μs)

        // 데이터 수신
        unsigned char response[256];  // 응답 버퍼 크기
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

        std::cout << "Received data: ";
        for (ssize_t i = 0; i < totalBytesRead; ++i) {
            std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(response[i]) << ' ';
        }
        std::cout << std::endl;

        // 데이터에서 온도, 습도, 이산화탄소 농도 값 추출
        if (totalBytesRead >= 82 && sizeof(response) >= 82) { // Ensure we have enough data
            float temperature = bytesToFloat(response + 7);
            float humidity = bytesToFloat(response + 25);
            float co2Concentration = bytesToFloat(response + 79);

            std::cout << "Temperature: " << temperature << " °C" << std::endl;
            std::cout << "Humidity: " << humidity << " %" << std::endl;
            std::cout << "CO2 Concentration: " << co2Concentration << " ppm" << std::endl;
            
            // insert data to db
            insertDataToDB(temperature, humidity, co2Concentration);
        } else {
            std::cerr << "Insufficient data received" << std::endl;
        }

        // 20분 대기 (300000000μs)
        usleep(1200000000);
    }

    close(serialPort);
    return 0;
}