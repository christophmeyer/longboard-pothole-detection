#include "camera.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "imu.h"
#include "sdcard.h"

static const int imu_queue_len = 200;
static const int camera_queue_len = 5;
static const bool record_imu_data = false;
static QueueHandle_t imu_queue;
static QueueHandle_t camera_queue;
static const char* TAG = "main";

// Task that reads data from the IMU and camera queues and writes it to the
// sdcard. For each camera queue item it reads up to 50 items from the IMU queue
// to account for the fact that imu data is written to the queue more often
// than the camera data.
void write_data(int argc, char* argv[]) {
  char capture_dir[14];
  imu_data imu_data;
  camera_data camera_data;
  init_sdcard();
  // Created a new capture_dir with incremented directory name
  init_capture_dir(capture_dir, true);
  char filename[36];
  sprintf(filename, "%s/gyrodata.csv", capture_dir);

  // Main loop that reads from the queues and writes to the sdcard.
  while (1) {
    // Taking IMU data from queue
    if (record_imu_data) {
      // Tries 50 times to read data from the imu queue, before
      // proceeding with the camera.
      for (int i = 0; i < 50; i++) {
        if (xQueueReceive(imu_queue, &imu_data, 0) == pdTRUE) {
          // Write imu record to csv.
          FILE* file = fopen(filename, "a");
          if (file != NULL) {
            char buffer[56];
            unsigned int len = sprintf(
                buffer, "%s;%d;%d;%d;%d;%d;%d\n", imu_data.timestamp,
                imu_data.acc_raw.x, imu_data.acc_raw.y, imu_data.acc_raw.z,
                imu_data.gyro_raw.x, imu_data.gyro_raw.y, imu_data.gyro_raw.z);

            size_t err = fwrite(buffer, sizeof(char), len, file);
          }
          fclose(file);
        }
      }
    }
    // Taking camera data from queue.
    if (xQueueReceive(camera_queue, &camera_data, 0) == pdTRUE) {
      char filename[36];
      sprintf(filename, "%s/capture_%s.gs", capture_dir, camera_data.timestamp);
      // Write camera data to sdcard.
      FILE* file = fopen(filename, "w");
      if (file != NULL) {
        size_t err = fwrite(camera_data.framebuffer.buf, 1,
                            camera_data.framebuffer.len, file);
      }
      fclose(file);
    }
  }
}

// Task that reads imu data every 20ms and puts it on the imu queue.
void read_imu_data(int argc, char* argv[]) {
  // Initialize the IMU
  MPU_t mpu;
  i2c0.begin(SDA, SCL, CLOCK_SPEED);
  init_imu(mpu, i2c0);
  imu_data data;

  while (true) {
    // Fetch new data every ~20ms.
    vTaskDelay(20 / portTICK_PERIOD_MS);
    mpu.acceleration(&data.acc_raw);
    mpu.rotation(&data.gyro_raw);

    // add the timestamp (time since boot) to the imu_data struct.
    int32_t ms_since_boot = (int32_t)(esp_timer_get_time() / 1000.);
    millis_to_timestamp(ms_since_boot, data.timestamp);

    // Put new datapoint on the queue.
    xQueueSend(imu_queue, &data, 0);
  }
}

// Task that reads a frame from the camera and puts it on the camera queue.
void read_camera_data(int argc, char* argv[]) {
  init_camera();
  camera_data data;
  while (true) {
    camera_fb_t* fb = esp_camera_fb_get();
    data.framebuffer = *fb;
    // Convert timeeval object to timestamp string that is used for the filename
    // of the image.
    timeval_to_timestamp(data.framebuffer.timestamp, data.timestamp);
    // Put new image on the queue.
    xQueueSend(camera_queue, &data, 10);
    esp_camera_fb_return(fb);
  }
}

extern "C" void app_main() {
  // Initialize the camera and imu queue
  imu_queue = xQueueCreate(imu_queue_len, sizeof(imu_data));
  camera_queue = xQueueCreate(camera_queue_len, sizeof(camera_data));

  // Start the two data capture tasks (imu and camera) on one core and the
  // task to write the data on the other core of the ESP32.
  xTaskCreatePinnedToCore((TaskFunction_t)&write_data, "write_data", 32 * 1024,
                          NULL, 1, NULL, 0);
  if (record_imu_data) {
    xTaskCreatePinnedToCore((TaskFunction_t)&read_imu_data, "data_capture_imu",
                            32 * 1024, NULL, 2, NULL, 1);
  }
  xTaskCreatePinnedToCore((TaskFunction_t)&read_camera_data,
                          "data_capture_camera", 32 * 1024, NULL, 1, NULL, 1);
  vTaskDelete(NULL);
}
