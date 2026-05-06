#include "ui.h"

volatile float Kp = 20.0; 
volatile float Ki = 16.0;
volatile float Kd = 6.0;

volatile bool enviar_kp = false;
volatile bool enviar_ki = false;
volatile bool enviar_kd = false;
volatile bool enviar_set_point = false;
volatile bool sistema_activo = false;

volatile bool encender_motor_izquierdo = false;
volatile bool encender_motor_derecho = false;

volatile int valor_setpoint = 0;

// Buffer global para evitar stack innecesario
static char buffer[10];


// ================== FUNCIONES AUXILIARES ==================
void set_label_float(lv_obj_t *label, float value)
{
    dtostrf(value, 4, 2, buffer);
    lv_label_set_text(label, buffer);
}


// ================== EVENTOS BOTONES ==================
void enviarProporcional(lv_event_t * e)
{
  enviar_kp = true;
}

void enviarIntegral(lv_event_t * e)
{
  enviar_ki = true;
}

void enviarDerivativo(lv_event_t * e)
{
  enviar_kd = true;
}

void enviarSetPoint(lv_event_t * e)
{
  enviar_set_point = true;
}


// ================== RESET ==================
void resetearconstantes(lv_event_t * e)
{
    // Kp
    sistema_activo = true;
    lv_slider_set_value(ui_SliderProporcional, 20, LV_ANIM_OFF);
    Kp = 20 * 0.05f;
    set_label_float(ui_LabelProporcional, Kp);

    // Ki
    lv_slider_set_value(ui_SliderIntegral, 16, LV_ANIM_OFF);
    Ki = 16 * 0.05f;
    set_label_float(ui_LabelIntegral, Ki);

    // Kd
    lv_slider_set_value(ui_SliderDerivativo, 6, LV_ANIM_OFF);
    Kd = 6 * 0.05f;
    set_label_float(ui_LabelDerivativo, Kd);

    enviar_kp = true;
    enviar_ki = true;
    enviar_kd = true;
}


// ================== SWITCHES ==================
void encendermotorizquierdo(lv_event_t * e)
{
    lv_obj_t * sw = lv_event_get_target(e);
    encender_motor_izquierdo = lv_obj_has_state(sw, LV_STATE_CHECKED);
}

void encendermotorderecho(lv_event_t * e)
{
    lv_obj_t * sw = lv_event_get_target(e);
    encender_motor_derecho = lv_obj_has_state(sw, LV_STATE_CHECKED);
}


// ================== SLIDERS ==================
void ui_event_SliderProporcional(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED)
    {
        int val = lv_slider_get_value(lv_event_get_target(e));

        Kp = val * 0.05f;

        set_label_float(ui_LabelProporcional, Kp);
    }
}

void ui_event_SliderIntegral(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED)
    {
        int val = lv_slider_get_value(lv_event_get_target(e));

        Ki = val * 0.05f;

        set_label_float(ui_LabelIntegral, Ki);
    }
}

void ui_event_SliderDerivativo(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED)
    {
        int val = lv_slider_get_value(lv_event_get_target(e));

        Kd = val * 0.05f;

        set_label_float(ui_LabelDerivativo, Kd);
    }
}


// ================== SETPOINT ==================
void ui_event_SlidersetPoint(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED)
    {
        valor_setpoint = lv_slider_get_value(lv_event_get_target(e));

        lv_label_set_text_fmt(ui_LabelValorSetPoint, "%d", valor_setpoint);
    }
}
