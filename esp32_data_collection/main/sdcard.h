#include "esp_system.h"

void init_capture_dir(char *capture_dir, bool create_dir);
void init_sdcard();
void millis_to_timestamp(int32_t millis, char *buffer);
void timeval_to_timestamp(timeval timestamp_in, char *timestamp_out);