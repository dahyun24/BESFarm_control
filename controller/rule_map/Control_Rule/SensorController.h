#ifndef SENSOR_CONTROLLER_H
#define SENSOR_CONTROLLER_H

#include <mysql/mysql.h>
#include <string>
#include <termios.h>

class SensorController {
public:
    SensorController(const char* server, const char* user, const char* password, const char* database);
    ~SensorController();

    void run();

private:
    bool isFanOn;
    bool isWindowOpen;
    bool isCoolingOn;
    bool isHeatingOn;

    MYSQL* conn;
    const char* portName;

    void connectToDatabase(const char* server, const char* user, const char* password, const char* database);
    void disconnectFromDatabase();
    float bytesToFloat(unsigned char* bytes);
    void insertDataToDB(float temperature, float humidity, float co2Concentration);
    void syncControlStateToDB();
    void checkAndControl(float temp, float humidity, int co2);
    void readSensorDataAndControl();
};

#endif // SENSOR_CONTROLLER_H