#include "camera.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "pothole_model.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

static const int predictions_queue_len = 1;
static const float beeper_threshold = 0.5;
static QueueHandle_t predictions_queue;
static const char* TAG = "main";

void run_inference(int argc, char* argv[]) {
  // Setup tflite model
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;

  constexpr int kTensorArenaSize = 140 * 1024;
  static uint8_t tensor_arena[kTensorArenaSize];

  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(pothole_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  // Setup camera
  init_camera();

  // Main loop fetching images and running inference on them.
  while (1) {
    // Capture image
    ESP_LOGI(TAG, "starting image capture");
    camera_fb_t* fb = esp_camera_fb_get();

    // Convert uint8 image to int8 since model expects int8.
    for (int i = 0; i < fb->len; i++) {
      input->data.int8[i] = (int8_t)((int)fb->buf[i] - 128);
    }

    esp_camera_fb_return(fb);

    // Run inference on image
    ESP_LOGI(TAG, "running inference");
    if (kTfLiteOk != interpreter->Invoke()) {
      TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed.");
    }
    int8_t p_bump_quant = output->data.int8[1];
    float p_bump =
        (p_bump_quant - output->params.zero_point) * output->params.scale;
    ESP_LOGI(TAG, "p(bump): %f", p_bump);

    // Put the prediction on the predictions queue for the buzzer task.
    xQueueSend(predictions_queue, &p_bump, 0);
  }
}

void run_buzzer(int argc, char* argv[]) {
  float p_bump;
  gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT);

  while (1) {
    if (xQueueReceive(predictions_queue, &p_bump, 0) == pdTRUE) {
      if (p_bump > beeper_threshold) {
        gpio_set_level(GPIO_NUM_16, 1);
      } else {
        gpio_set_level(GPIO_NUM_16, 0);
      }
    }
  }
}

extern "C" void app_main() {
  // Setup queue for predictions
  predictions_queue = xQueueCreate(predictions_queue_len, sizeof(float));

  // Start inference and buzzer task on separate cores.
  xTaskCreatePinnedToCore((TaskFunction_t)&run_inference, "run_inference",
                          32 * 1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore((TaskFunction_t)&run_buzzer, "run_buzzer", 32 * 1024,
                          NULL, 1, NULL, 1);
  vTaskDelete(NULL);
}
