#include <ESP32Servo.h>
#include <Arduino.h>

// Instanciando os servos
Servo SrvDir;   // Servo da direção
Servo EscMtr;   // Motor da locomoção
Servo SrvBrk;   // Servo do freio
Servo SrvGear;  // Servo da marcha

// Protótipo da função de configuração do ESC
void configESC();

void setup()
{
  Serial.begin(115200);

  SrvDir.attach(19);  // Servo da direção
  EscMtr.attach(16);  // Motor da locomoção (65 a 450 andando / Parado = 0)
  SrvBrk.attach(17);  // Servo do freio (41 freia / 141 solta)
  SrvGear.attach(18); // Servo da marcha (60 Ré / 200 Frente)

  configESC();

  EscMtr.write(0);
  SrvDir.write(92);
  SrvBrk.write(141);
  SrvGear.write(200);
  
  delay(500);
}

void loop()
{
  EscMtr.write(65);
  SrvDir.write(92);
  SrvBrk.write(141);
  SrvGear.write(200);

  delay(3000);

  EscMtr.write(00);
  SrvDir.write(141);
  SrvBrk.write(41);
  SrvGear.write(200);

  delay(30000);
}

void configESC()
{
  // Início da configuração do ESC
  EscMtr.write(450);
  // Aguarda 35,8 segundos para chegar ao modo default do ESC
  delay(35800);
  
  // Descida de valores até 0 para configurar o ESC
  for (int i = 450; i >= 0; i--)
  { 
    Serial.println(i);
    EscMtr.write(i);
    delay(5);
  }
  delay(5000);

  // Fim da configuração do ESC
  SrvDir.write(92);
  SrvBrk.write(141);
  SrvGear.write(200);
}