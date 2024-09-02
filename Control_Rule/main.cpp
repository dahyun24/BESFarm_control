#include <iostream>
#include <mysql/mysql.h>
#include <ctime>
#include <unistd.h>
#include "prepare_message.h"
#include "write_node.h"

bool isFanOn = false;
bool isWindowOpen = false;
bool isCoolingOn = false;
bool isHeatingOn = false;

void checkAndControl(float temp, float humidity, int co2) {
    // 환풍기 제어
    if (co2 >= 1000 && !isFanOn) {
        std::cout << "환풍기 ON" << std::endl;
        isFanOn = true;
    } else if (co2 <= 800 && isFanOn) {
        std::cout << "환풍기 OFF" << std::endl;
        isFanOn = false;
    } else{
        std::cout << "환풍기 상태 유지" << std::endl;
    }

    // 천창 제어
    if (humidity >= 70 && !isWindowOpen) {
        std::cout << "천창 OPEN" << std::endl;
        isWindowOpen = true;
    } else if (humidity <= 50 && isWindowOpen) {
        std::cout << "천창 CLOSE" << std::endl;
        isWindowOpen = false;
    }else{
        std::cout << "천창 상태 유지" << std::endl;
    }

    // 냉난방기 제어
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
    } else{
        std::cout << "냉난방기 상태 유지" << std::endl;
    }
}

void printStatus() {
    std::time_t now = std::time(nullptr);

    // 제어 상태와 현재 시간을 메시지로 준비
    std::string message = prepareMessage(isFanOn, isWindowOpen, isCoolingOn, isHeatingOn, now);

    // 메시지를 구동기 노드로 전달
    writeNode(isFanOn, isWindowOpen, isCoolingOn, isHeatingOn, now);
}

int main() {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    const char *server = "192.168.0.68";
    const char *user = "besf";
    const char *password = "besf0905";
    const char *database = "sensor_data";

    conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        std::cerr << "MySQL connection failed: " << mysql_error(conn) << std::endl;
        return 1;
    }

    while (true) {
        if (mysql_query(conn, "SELECT temp, humidity, co2, date FROM sensor ORDER BY date DESC LIMIT 1")) {
            std::cerr << "SELECT query failed: " << mysql_error(conn) << std::endl;
            mysql_close(conn);
            return 1;
        }

        res = mysql_store_result(conn);
        if (res == NULL) {
            std::cerr << "mysql_store_result failed: " << mysql_error(conn) << std::endl;
            mysql_close(conn);
            return 1;
        }

        row = mysql_fetch_row(res);
        if (row != NULL) {
            // 데이터 출력
            std::cout << "Received data:" << std::endl;
            std::cout << "Date: " << row[3] << std::endl;
            std::cout << "Temperature: " << row[0] << " °C" << std::endl;
            std::cout << "Humidity: " << row[1] << " %" << std::endl;
            std::cout << "CO2: " << row[2] << " ppm" << std::endl;

            float temp = std::stof(row[0]);
            float humidity = std::stof(row[1]);
            int co2 = std::stoi(row[2]);

            checkAndControl(temp, humidity, co2);

            printStatus();
        } else {
            std::cout << "No data found" << std::endl;
        }

        mysql_free_result(res);

        sleep(1200); // 20분 대기
    }

    mysql_close(conn);
    return 0;
}
