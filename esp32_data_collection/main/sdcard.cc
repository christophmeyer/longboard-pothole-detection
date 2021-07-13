#include "sdcard.h"

#include "driver/sdmmc_host.h"
#include "esp_camera.h"

extern "C" {
#include "esp_vfs.h"
}
#include <ctype.h>

#include "esp_log.h"
#include "esp_vfs_fat.h"

static const char *TAG = "sdcard";

bool is_number(char *string) {
  for (int i = 0; i < strlen(string); i++) {
    if (!isdigit(string[i])) {
      return false;
    }
  }
  return true;
}

void init_capture_dir(char *capture_dir, bool create_dir) {
  struct dirent *ent;
  char mountpoint[] = "/sdcard";
  DIR *dir = opendir(mountpoint);
  int max_dir = 1;
  if (dir != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if ((ent->d_type == DT_DIR) && is_number(ent->d_name)) {
        if (atoi(ent->d_name) > max_dir) {
          max_dir = atoi(ent->d_name);
        }
      }
    }
  }
  max_dir += 1;
  sprintf(capture_dir, "/sdcard/%05d", max_dir % 10000);
  if (create_dir) {
    int result = mkdir(capture_dir, 0777);
  }
}

void init_sdcard() {
  esp_err_t ret = ESP_FAIL;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.flags = SDMMC_HOST_FLAG_1BIT;
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 1,
  };
  sdmmc_card_t *card;

  ESP_LOGI(TAG, "Mounting SD card...");
  ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config,
                                &card);

  gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_4, 0);

  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "SD card mount successfully!");
  } else {
    ESP_LOGE(TAG, "Failed to mount SD card VFAT filesystem. Error: %s",
             esp_err_to_name(ret));
  }
}

void millis_to_timestamp(int32_t millis, char *buffer) {
  int ms = millis % 1000;
  int s = (millis / 1000) % 60;
  int m = (millis / (60000)) % 60;
  int h = (millis / (3600000)) % 24;
  sprintf(buffer, "%02d%02d%02d%03d", h, m, s, ms);
}

void timeval_to_timestamp(timeval timestamp_in, char *timestamp_out){
  unsigned long total_ms = (timestamp_in.tv_sec * 1000) + (timestamp_in.tv_usec / 1000);

  int ms = total_ms % 1000;
  int s = (total_ms / 1000) % 60;
  int m = (total_ms / (60000)) % 60;
  int h = (total_ms / (3600000)) % 24;
  sprintf(timestamp_out, "%02d%02d%02d%03d", h, m, s, ms);
}