#include <Arduino.h>
#include <FreeRTOS.h>
#include "semphr.h"
#include <task.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "eez-project/ui.h"

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */

static void guiTask(void *pvParameter);

/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore;

/*Change to your screen resolution*/
static const uint32_t screenWidth  = 128;
static const uint32_t screenHeight = 160;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * 10 ];

void gui_task_init(void)
{
    /* If you want to use a task to create the graphic, you NEED to create a Pinned task
     * Otherwise there can be problem such as memory corruption and so on.
     * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
    /* TODO: ！！！史前巨坑！！！优先级要改大点 */
    xTaskCreate(guiTask, "gui", 4096 * 2, NULL, 10, NULL);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
   uint32_t w = ( area->x2 - area->x1 + 1 );
   uint32_t h = ( area->y2 - area->y1 + 1 );

   tft.startWrite();
   tft.setAddrWindow( area->x1, area->y1, w, h );
   tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
   tft.endWrite();

   lv_disp_flush_ready( disp );
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
  data->state = LV_INDEV_STATE_REL;

  //  uint16_t touchX, touchY;

  //  bool touched = tft.getTouch( &touchX, &touchY, 600 );

  //  if( !touched )
  //  {
  //     data->state = LV_INDEV_STATE_REL;
  //  }
  //  else
  //  {
  //     data->state = LV_INDEV_STATE_PR;

  //     /*Set the coordinates*/
  //     data->point.x = touchX;
  //     data->point.y = touchY;

  //     Serial.print( "Data x " );
  //     Serial.println( touchX );

  //     Serial.print( "Data y " );
  //     Serial.println( touchY );
  //  }
}

static void guiTask(void *pvParameter)
{
    (void)pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();

   lv_init();

   tft.begin();          /* TFT init */
   tft.setRotation( 2 ); /* Landscape orientation, flipped */

   /*Set the touchscreen calibration data,
     the actual data for your display can be aquired using
     the Generic -> Touch_calibrate example from the TFT_eSPI library*/
  //  uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
  //  tft.setTouch( calData );

   lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * 10 );

   /*Initialize the display*/
   static lv_disp_drv_t disp_drv;
   lv_disp_drv_init( &disp_drv );
   /*Change the following line to your display resolution*/
   disp_drv.hor_res = screenWidth;
   disp_drv.ver_res = screenHeight;
   disp_drv.flush_cb = my_disp_flush;
   disp_drv.draw_buf = &draw_buf;
   lv_disp_drv_register( &disp_drv );

   /*Initialize the (dummy) input device driver*/
   static lv_indev_drv_t indev_drv;
   lv_indev_drv_init( &indev_drv );
   indev_drv.type = LV_INDEV_TYPE_POINTER;
   indev_drv.read_cb = my_touchpad_read;
   lv_indev_drv_register( &indev_drv );

   // Set to 1 if you do not see any GUI
#if 0
   /* Create simple label */
   lv_obj_t *label = lv_label_create( lv_scr_act() );
   lv_label_set_text( label, "Hello Arduino! (V8.3.0)" );
   lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );
#else
   // EEZ GUI init
   ui_init();
#endif
   Serial.println( "UI created" );

    while (1) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(1));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            ui_tick();
            xSemaphoreGive(xGuiSemaphore);
        }
    }

    /* A task should NEVER return */
    vTaskDelete(NULL);
}

void setup()
{
   Serial.begin( 115200 ); /* prepare for possible serial debug */
   Serial.println( "Hello RP2040! (V8.3.0)" );

   gui_task_init();
}

void loop()
{
   lv_tick_inc(1);
   vTaskDelay(pdMS_TO_TICKS(1));
}