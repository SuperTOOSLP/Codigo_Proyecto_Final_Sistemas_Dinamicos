#include <Arduino.h>
#include "angulo.c"
#include "evitar_desborde_integral.c"

#define motor_izq 3
#define motor_der 6

int potenciometro = A0;
int valor = 0;
bool sistema_activo = false;
char buffer[20]; // Un poco más grande por seguridad

double tiempo = 0;
double tiempo_pasado = 0;
double set_point = 0.0; 
double t_pas = 0;
int angulo = 0;

double Kp = 1.0;
double Ki = 0.8;
double Kd = 0.3;

double error = 0.0, error_anterior = 0.0;
double integral = 0.0, derivada = 0.0, salida = 0.0, dt = 0.0;

int PWM_base = 120;

void setup() {
  Serial.begin(115200);
  pinMode(motor_izq, OUTPUT);
  pinMode(motor_der, OUTPUT);
}

void loop() {
  tiempo = millis();
  dt = (tiempo - tiempo_pasado) / 1000.0;

  /// Lectura hecha de serial hecha con IA
  if (Serial.available() > 0) {
    int len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
    buffer[len] = '\0'; 

    if (strncmp(buffer, "SC:", 3) == 0) {
      int val_sc = atoi(&buffer[3]);
      if (val_sc == 1) {
        sistema_activo = true;
      } else {
        sistema_activo = false;
        integral = 0; 
        analogWrite(motor_izq, 0); 
        analogWrite(motor_der, 0);
      }
    }
    else if (strncmp(buffer, "MI:", 3) == 0) {
      int val_mi = atoi(&buffer[3]);
      if (!sistema_activo) {
        analogWrite(motor_izq, (val_mi == 1) ? 120 : 0);
      }
    }
    else if (strncmp(buffer, "MD:", 3) == 0) {
      int val_md = atoi(&buffer[3]);
      if (!sistema_activo) {
        analogWrite(motor_der, (val_md == 1) ? 120 : 0);
      }
    }
    else if (strncmp(buffer, "Kp:", 3) == 0) {
      Kp = atof(&buffer[3]);
    } 
    else if (strncmp(buffer, "Ki:", 3) == 0) {
      Ki = atof(&buffer[3]);
    } 
    else if (strncmp(buffer, "Kd:", 3) == 0) {
      Kd = atof(&buffer[3]);
    } 
    else if (strncmp(buffer, "SP:", 3) == 0) {
      set_point = atof(&buffer[3]);
    }
    else if (isdigit(buffer[0]) || (buffer[0] == '-' && isdigit(buffer[1]))) {
      set_point = atof(buffer);
    }
  }

  /// Control PID
  if (dt >= 0.05) {
        valor = analogRead(potenciometro);
        angulo = obtener_angulo(valor);

    if (sistema_activo) {
      error = set_point - angulo;
      derivada = (error - error_anterior) / dt;
      integral += (error + error_anterior) / 2 * dt;
      integral = acotar_integral(integral);

      salida = (Kp * error) + (Ki * integral) + (Kd * derivada);

      int pwm_izq = constrain(PWM_base + salida, 0, 255);
      int pwm_der = constrain(PWM_base - salida, 0, 255);

      analogWrite(motor_izq, pwm_izq);
      analogWrite(motor_der, pwm_der);
    } 
    else {
      /// forzar motores 0 (sistema apagado)
      // Nota: Si no hay comando MI/MD reciente, se mantienen en 0 por el comando SC:0
      error_anterior = 0;
      integral = 0;
    }

    error_anterior = error;
    tiempo_pasado = tiempo;
  }


  if((millis() - t_pas) > 150){
    Serial.println(angulo);
    t_pas = millis();
  }

}