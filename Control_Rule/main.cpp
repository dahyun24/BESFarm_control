#include "SensorController.h"

int main() {
    const char* server = "192.168.0.68";
    const char* user = "besf";
    const char* password = "besf0905";
    const char* database = "sensor_data";

    SensorController controller(server, user, password, database);
    controller.run();

    return 0;
}
