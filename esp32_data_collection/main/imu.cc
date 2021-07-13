#include "imu.h"

#include "esp_log.h"

static const char *TAG = "imu";

void init_imu(MPU_t MPU, I2C_t &i2c0) {
  // Initialize I2C connection to IMU.
  MPU.setBus(i2c0);
  MPU.setAddr(mpud::MPU_I2CADDRESS_AD0_LOW);
  MPU.testConnection();
  MPU.initialize();

  // Configure IMU.
  MPU.setSampleRate(250);
  MPU.setAccelFullScale(mpud::ACCEL_FS_4G);
  MPU.setGyroFullScale(mpud::GYRO_FS_500DPS);
  MPU.setInterruptEnabled(mpud::INT_EN_RAWDATA_READY);
}

void save_imu_data(MPU_t MPU, char *capture_dir, char *timestamp) {
  // Fetch datapoint from IMU.
  mpud::raw_axes_t accelRaw;
  mpud::raw_axes_t gyroRaw;
  MPU.acceleration(&accelRaw);
  MPU.rotation(&gyroRaw);

  char filename[36];
  sprintf(filename, "%s/gyrodata.csv", capture_dir);

  // Append datapoint to the gyrodata.csv.
  FILE *file = fopen(filename, "a");
  if (file != NULL) {
    char buffer[56];
    unsigned int len =
        sprintf(buffer, "%s;%d;%d;%d;%d;%d;%d\n", timestamp, accelRaw.x,
                accelRaw.y, accelRaw.z, gyroRaw.x, gyroRaw.y, gyroRaw.z);
    size_t err = fwrite(buffer, sizeof(char), len, file);
    ESP_LOGI(TAG, "IMU data saved");
  } else {
    ESP_LOGE(TAG, "Could not open file");
  }
  fclose(file);
}