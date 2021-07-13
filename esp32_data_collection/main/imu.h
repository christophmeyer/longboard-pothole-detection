#include "I2Cbus.hpp"
#include "MPU.hpp"

// static constexpr gpio_num_t SDA = GPIO_NUM_13;
// static constexpr gpio_num_t SCL = GPIO_NUM_15;
static constexpr gpio_num_t SDA = GPIO_NUM_3;
static constexpr gpio_num_t SCL = GPIO_NUM_1;
static constexpr uint32_t CLOCK_SPEED = 400000;

typedef struct imu_data {
  mpud::raw_axes_t acc_raw;
  mpud::raw_axes_t gyro_raw;
  char timestamp[10];
} imu_data;

void init_imu(MPU_t MPU, I2C_t &i2c0);
void save_imu_data(MPU_t MPU, char *capture_dir, char *timestamp);