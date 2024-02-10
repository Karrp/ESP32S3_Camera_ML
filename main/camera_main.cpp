extern "C"{

#define WIFI_SSID       "WIFI_DSP"
#define WIFI_PASSWORD   "MNIST_DSP"
#define WIFI_CHANNEL    7
#define MAX_STA_CONN    4

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// support IDF 5.x
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

// ----------- CAMERA -----------
#include "esp_camera.h"

#define BOARD_ESP_S3_EYE

// ESP32S3 EYE
#ifdef BOARD_ESP_S3_EYE
// #define CAMERA_MODULE_NAME "ESP-S3-EYE"
    #define CAM_PIN_PWDN -1
    #define CAM_PIN_RESET -1

    #define CAM_PIN_VSYNC 6
    #define CAM_PIN_HREF 7
    #define CAM_PIN_PCLK 13
    #define CAM_PIN_XCLK 15

    #define CAM_PIN_SIOD 4
    #define CAM_PIN_SIOC 5

    #define CAM_PIN_D0 11
    #define CAM_PIN_D1 9
    #define CAM_PIN_D2 8
    #define CAM_PIN_D3 10
    #define CAM_PIN_D4 12
    #define CAM_PIN_D5 18
    #define CAM_PIN_D6 17
    #define CAM_PIN_D7 16

#endif

static const char *photo_TAG = "Take photo:";
static const char *process_img_TAG = "Photo preprocessing:";
static const char *wifi_TAG = "WiFi handler:";

#if ESP_CAMERA_SUPPORTED
    static camera_config_t camera_config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
        .xclk_freq_hz = 10000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_GRAYSCALE, //YUV422,GRAYSCALE,RGB565,JPEG
        .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

        .jpeg_quality = 63, //0-63, for OV series camera sensors, lower number means higher quality
        .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    static esp_err_t init_camera(void)
    {
        //initialize the camera
        esp_err_t err = esp_camera_init(&camera_config);
        if (err != ESP_OK)
        {
            ESP_LOGE(photo_TAG, "Camera Init Failed");
            return err;
        }

        return ESP_OK;
    }
#endif

// ----------- IMAGE PREPROCESSING -------------
// #include <stdio.h>
// #include <stdlib.h>
// #include <stddef.h>
// #include "driver/gpio.h"

// #include "tensorflow/lite/c/common.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/kernels/micro_ops.h"
// #include "tensorflow/lite/schema/schema_generated.h"
// #include "tensorflow/lite/micro/micro_op_resolver.h"

// #include "tensorflow/lite/micro/micro_common.h"

// #define MODEL_IMAGE_WIDTH 32
// #define MODEL_IMAGE_HEIGHT 32
// #define NUM_CHANNELS 1

// namespace {
//     tflite::ErrorReporter* error_reporter = nullptr;
//     const tflite::Model* model = nullptr;
//     tflite::MicroInterpreter* interpreter = nullptr;
//     TfLiteTensor* model_input = nullptr;

//     // Create an area of memory to use for input, output, and intermediate arrays.
//     // The size of this depends on the model and may need to be
//     // determined by experimentation.
//     constexpr int kTensorArenaSize = 60 * 1024;
//     uint8_t tensor_arena[kTensorArenaSize];
//     float* model_input_buffer = nullptr;
// }  // namespace

// void process_image(camera_fb_t *fb)
// {
//     //Serial.println("Classify image...");
//     // allocate space for cropped image
//     int img_size = MODEL_IMAGE_WIDTH * MODEL_IMAGE_HEIGHT * NUM_CHANNELS;
//     uint8_t * tmp_buffer = (uint8_t *) malloc(img_size);
//     //Serial.println("TMP Buffer");
//     ESP_LOGI(process_img_TAG, "TMP Buffer");
//     // resize image
//     image_resize_linear(tmp_buffer,fb->buf,MODEL_IMAGE_HEIGHT,MODEL_IMAGE_WIDTH,NUM_CHANNELS,fb->width,fb->height);
//     ESP_LOGI(process_img_TAG, "Image resized");


//     // normalize image
//     ESP_LOGI(process_img_TAG, "Size: %d", sizeof(tmp_buffer));
//     for (int i=0; i < img_size; i++)
//     {
//         //printf("Data %i", tmp_buffer[i]);
//         //(interpreter->input(0))->data.f[i] = tmp_buffer[i] / 255.0f;
//         model_input_buffer[i] = tmp_buffer[i] / 255.0f;
//         //normalise_image_buffer( (interpreter->input(0))->data.f, tmp_buffer, img_size);
//     }

//     free(tmp_buffer);
//     ESP_LOGI(process_img_TAG, "Invoking interpreter");
//     //Serial.println("Invoking interpreter");
//     if (kTfLiteOk != interpreter->Invoke()) 
//         {
//                     error_reporter->Report("Error");
//     }

//     ESP_LOGI(process_img_TAG, "Showing results");
//     TfLiteTensor* output = interpreter->output(0);
                
//                     for (int i=1; i < kCategory; i++)
//                     {
//             ESP_LOGI(process_img_TAG, "Label=%s, Prob=%f",kCategoryLabels[i], output->data.f[i] );
        
//                     }

//     esp_camera_fb_return(fb);
//     fb = NULL;
//     ESP_LOGI(process_img_TAG, "Free Image");
// }


// ----------- NEURAL NETWORK -------------


// ------------- WEB SERVER ---------------
/*
#include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"


#define EXAMPLE_ESP_WIFI_SSID      WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       MAX_STA_CONN

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(wifi_TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(wifi_TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .authmode = WIFI_AUTH_WPA3_PSK,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .pmf_cfg = {
                    .required = true,
            },            
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,                 
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(wifi_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}
*/

// ---------------- MAIN ------------------


void app_main(void)
{
#if ESP_CAMERA_SUPPORTED
    if(ESP_OK != init_camera()) {
        return;
    }

    /* //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(wifi_TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    */

    while (1)
    {

        ESP_LOGI(photo_TAG, "Taking picture...");
        camera_fb_t *pic = esp_camera_fb_get();

        // use pic->buf to access the image
        ESP_LOGI(photo_TAG, "Picture taken! Its size was: %zu bytes", pic->len);

        if(!pic)
        {
            ESP_LOGI(photo_TAG, "Picture problem, no image processing");
        }
        else
        {
            // process_image(pic);
        }

        esp_camera_fb_return(pic); // Release data space

        vTaskDelay(10000 / portTICK_RATE_MS);
    }
#else
    ESP_LOGE(photo_TAG, "Camera support is not available for this chip");
    return;
#endif
}
}
