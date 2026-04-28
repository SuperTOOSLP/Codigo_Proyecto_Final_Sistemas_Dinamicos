#include "pins_config.h"
#include "LovyanGFX_Driver.h"

#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include <SPI.h>

#include <stdbool.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#include "ui.h"

/* Expand IO */
#include <TCA9534.h>
TCA9534 ioex;

LGFX gfx;

extern "C"{
extern volatile float Kp;
extern volatile float Ki;
extern volatile float Kd;

extern volatile bool enviar_kp;
extern volatile bool enviar_ki;
extern volatile bool enviar_kd;
extern volatile bool enviar_set_point;

extern volatile bool encender_motor_izquierdo;
extern volatile bool encender_motor_derecho;

extern volatile int valor_setpoint;
}

static bool estado_anterior_MI = false;
static bool estado_anterior_MD = false;

// Punteros para las series de la gráfica
lv_chart_series_t * ui_ser_angulo; 
lv_chart_series_t * ui_ser_setpoint;


/* Change to your screen resolution */
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf;
static lv_color_t *buf1;

uint16_t touch_x, touch_y;

//  Display refresh
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  if (gfx.getStartCount() > 0) {
    gfx.endWrite();
  }
  gfx.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::rgb565_t *)&color_p->full);

  lv_disp_flush_ready(disp);  //	Tell lvgl that the refresh is complete
}

//  Read touch
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
  data->state = LV_INDEV_STATE_REL;// The state of data existence when releasing the finger
  bool touched = gfx.getTouch( &touch_x, &touch_y );
  if (touched)
  {
    data->state = LV_INDEV_STATE_PR;

    //  Set coordinates
    data->point.x = touch_x;
    data->point.y = touch_y;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(1);

  pinMode(19, OUTPUT);

  Wire.begin(15, 16);
  delay(50);

  ioex.attach(Wire);
  ioex.setDeviceAddress(0x18);
  ioex.config(1, TCA9534::Config::OUT);
  ioex.config(2, TCA9534::Config::OUT);
  ioex.config(3, TCA9534::Config::OUT);
  ioex.config(4, TCA9534::Config::OUT);

  /* Turn on backlight*/
  ioex.output(1, TCA9534::Level::H);

  // GT911 power on timing ->Select 0x5D
  pinMode(1, OUTPUT);
  digitalWrite(1, LOW);
  ioex.output(2, TCA9534::Level::L);
  delay(20);
  ioex.output(2, TCA9534::Level::H);
  delay(100);
  pinMode(1, INPUT);
  /*end*/

  // Init Display
  gfx.init();
  gfx.initDMA();
  gfx.startWrite();
  gfx.fillScreen(TFT_BLACK);

  lv_init();
  size_t buffer_size = sizeof(lv_color_t) * LCD_H_RES * LCD_V_RES;
  buf = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  buf1 = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);

uint32_t buffer_pixels = LCD_H_RES * 100; // 100 líneas es más que suficiente

lv_disp_draw_buf_init(&draw_buf, buf, buf1, buffer_pixels);

  // Initialize display
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  // Change the following lines to your display resolution
  disp_drv.hor_res = LCD_H_RES;
  disp_drv.ver_res = LCD_V_RES;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Initialize input device driver program
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  delay(100);

  gfx.fillScreen(TFT_BLACK);
  // lv_demo_widgets();// Main UI interface
  ui_init();
  enviar_kp = false;
  enviar_ki = false;
  enviar_kd = false;

  // Obtenemos las series (suponiendo que la serie 1 es ángulo y la 2 es setpoint)
  // Nota: LVGL maneja las series por orden de creación
  ui_ser_angulo = lv_chart_get_series_next(ui_ChartGrafica, NULL);
  ui_ser_setpoint = lv_chart_get_series_next(ui_ChartGrafica, ui_ser_angulo);

  Serial.println( "Setup done Luis Pinto" );
}

void loop()
{
  lv_timer_handler(); /* let the GUI do its work */

  if (Serial.available()) {
    // Leemos el valor (ejemplo: "25\n")
    int angulo_actual = Serial.parseInt(); 
    
    // Validamos que el dato esté en el rango esperado para evitar picos raros
    if (angulo_actual >= -50 && angulo_actual <= 50) {
      
      // Actualizamos la serie del Ángulo (Línea Roja)
      lv_chart_set_next_value(ui_ChartGrafica, ui_ser_angulo, angulo_actual);
      
      // Actualizamos la serie del SetPoint (Línea Verde) 
      // para que se vea la referencia junto al ángulo
      int sp = lv_slider_get_value(ui_SlidersetPoint);
      lv_chart_set_next_value(ui_ChartGrafica, ui_ser_setpoint, sp);

      // Refrescamos la gráfica para que se vea el movimiento
      lv_chart_refresh(ui_ChartGrafica);
    }
  }

  if(enviar_kp){
    Serial.printf("Kp:%.2f\n", Kp);
    enviar_kp = false;
  }
  if(enviar_ki){
    Serial.printf("Ki:%.2f\n", Ki);
    enviar_ki = false;
  }
  if(enviar_kd){
    Serial.printf("Kd:%.2f\n", Kd);
    enviar_kd = false;
  }

  if(enviar_set_point){
    Serial.printf("SP:%d\n", valor_setpoint);
    enviar_set_point = false;
  }

if(encender_motor_izquierdo != estado_anterior_MI){
    if(encender_motor_izquierdo){
        Serial.println("MI:1");
    } else {
        Serial.println("MI:0");
    }
    estado_anterior_MI = encender_motor_izquierdo;
}

if(encender_motor_derecho != estado_anterior_MD){
    if(encender_motor_derecho){
        Serial.println("MD:1");
    } else {
        Serial.println("MD:0");
    }
    estado_anterior_MD = encender_motor_derecho;
}

  delay(1);
}
