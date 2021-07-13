#include "camera.h"

#include "esp_camera.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "camera";

void save_photo_timestamp(char *capture_dir, char *timestamp) {
  // Fetch image
  ESP_LOGI(TAG, "Taking picture: ");
  camera_fb_t *fb = esp_camera_fb_get();

  char filename[36];
  sprintf(filename, "%s/capture_%s.gs", capture_dir, timestamp);

  // Save image into the capture_dir
  FILE *file = fopen(filename, "w");
  if (file != NULL) {
    size_t err = fwrite(fb->buf, 1, fb->len, file);
    ESP_LOGI(TAG, "File saved: %s\n", filename);

  } else {
    ESP_LOGI(TAG, "Could not open file");
  }
  fclose(file);
  esp_camera_fb_return(fb);
}

void init_camera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_96X96;
  config.jpeg_quality = 10;
  config.fb_count = 2;
  // config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t cam_err = esp_camera_init(&config);
  if (cam_err != ESP_OK) {
    ESP_LOGE(TAG, "Camera init failed with error 0x%x", cam_err);
    return;
  }
}
