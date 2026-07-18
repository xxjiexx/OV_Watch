#ifndef APP_SENSOR_H
#define APP_SENSOR_H

typedef struct
{
  int heart_rate;
  int spo2;
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  int step;
  float angle;
} sensor_data_t;

extern sensor_data_t Sensor_data;

void App_Sensor_Read(void);

#endif
