#include <Arduino.h>
#include <ESP32Servo.h>
#include <Bluepad32.h>

// ======================================================
// SERVOS E CONSTANTES DO SEU HARDWARE
// ======================================================
Servo SrvDir;    // direção
Servo EscMtr;    // motor
Servo SrvBrk;    // freio
Servo SrvGear;   // marcha

// Direção
const int DIR_LEFT  = 41;
const int DIR_RIGHT = 141;
const int DIR_CENTER = 92;
int dirOffset = 13;     // Ajuste fino da direção (positivo=>direita / negativo=>esquerda)
const int DIR_OFFSET_STEP = 1;   // passo de aumento

// ESC
const int ESC_PPM = 66;
const int ESC_STOP = 20;
int escSpeed = ESC_PPM;      // velocidade atual
const int ESC_STEP = 1;      // passo de aumento
uint16_t lastDpad = 0;       // guarda estado anterior do DPad

// Freio
const int BRK_ON  = 45;
const int BRK_OFF = 141;

// Marcha
const int GEAR_REVERSE = 60;
const int GEAR_FORWARD = 200;


// ======================================================
// VARIÁVEIS DE ESTADO
// ======================================================
bool gearForward = true;

bool lastR1 = false;

// Ponteiro para o controle
GamepadPtr myGamepad;


// ======================================================
// BLUEPAD — CALLBACKS
// ======================================================
void onConnectedGamepad(GamepadPtr gp) {
  myGamepad = gp;
  myGamepad->setColorLED(0, 255, 0);
}

void onDisconnectedGamepad(GamepadPtr gp) {
  EscMtr.write(ESC_STOP);
  delay(200);
  SrvBrk.write(BRK_ON);
  if (myGamepad == gp) {
      myGamepad = nullptr;
  }
}


// ======================================================
// SETUP
// ======================================================
void setup() {
    Serial.begin(115200);

    // Servos
    SrvDir.attach(19);
    EscMtr.attach(32); //16 era o antigo que foi trocado por conta da comunicação serial
    SrvBrk.attach(17); //17 era o antigo que foi trocado por conta da comunicação serial
    SrvGear.attach(18);

    // Valores iniciais
    SrvDir.write(DIR_CENTER);
    SrvBrk.write(BRK_OFF);
    SrvGear.write(GEAR_FORWARD);

    // Bluepad
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

    // Para esquecer o controle caso necessário (Caso esteja dando problema, parear novamente resolve)
    //BP32.forgetBluetoothKeys();

    // Deixar o controle virtual desativado (O controle virtual é o touchpad do dualshock 4)
    BP32.enableVirtualDevice(false);

    Serial.println("Sistema iniciado!");
}


// ======================================================+
// FUNÇÃO PRINCIPAL DO LOOP
// ======================================================
void loop() {
    // Atualiza Bluepad32
    BP32.update();

    if (myGamepad && myGamepad->isConnected()) {

        // ================================
        // 1) DIREÇÃO — ANALÓGICO ESQUERDO
        // ================================
        // gp->axisLX vai de -511 (esq) a 511 (dir)
        int lx = myGamepad->axisX();  

        // Mapeamento: -511 → DIR_LEFT, +511 → DIR_RIGHT
        int dirValue = map(lx, -511, 511, DIR_LEFT, DIR_RIGHT) + dirOffset;
        dirValue = constrain(dirValue, DIR_LEFT, DIR_RIGHT);  // Garante que o valor fique sempre dentro do limite do servo
        SrvDir.write(dirValue);

        // ================================
        // 2) ACELERAÇÃO — BOTÃO X
        // ================================
        bool xPressed = myGamepad->a(); 

        if (xPressed) {
            SrvBrk.write(BRK_OFF);
            delay(200);
            EscMtr.write(escSpeed);
            myGamepad->setColorLED(0, 0, 255);
        }


        // ================================
        // 3) FREIO — BOTÃO O
        // ================================
        bool oPressed = myGamepad->b();

        if (oPressed) {
            EscMtr.write(ESC_STOP);
            delay(100);
            SrvBrk.write(BRK_ON);
            myGamepad->setColorLED(255, 0, 0);
            myGamepad->playDualRumble(0, 250, 0x80, 0x40);
            delay(500);
            SrvBrk.write(BRK_OFF);
            myGamepad->setColorLED(0, 255, 0);
        }

        // ================================
        // 4) MARCHA — R1 toggle
        // ================================
        bool r1Pressed = myGamepad->r1();

        if (r1Pressed && !lastR1) {
            gearForward = !gearForward;
            if (gearForward){
                SrvGear.write(GEAR_FORWARD);
            }
            else{
                SrvGear.write(GEAR_REVERSE);
            }
        }
        lastR1 = r1Pressed;


        // ================================
        // AJUSTE FINO DA DIREÇÃO — SETAS
        // ================================
        uint16_t dpad  = myGamepad->dpad();
 
        if (dpad == 0x08) {
            dirOffset -= DIR_OFFSET_STEP;
            myGamepad->setColorLED(255, 150, 0); 
            delay(200);
            myGamepad->setColorLED(0, 255, 0);
            myGamepad->playDualRumble(0, 250, 0x80,0x40);
        }

        if (dpad == 0x04) {
            dirOffset += DIR_OFFSET_STEP;
            myGamepad->setColorLED(0, 150, 255); 
            delay(200); 
            myGamepad->setColorLED(0, 255, 0);
            myGamepad->playDualRumble(0, 250, 0x80,0x40);
        }

        // ================================
        // CONTROLE DE VELOCIDADE
        // ================================

        // DPad UP -> aumenta velocidade
        if (dpad == 0x01 && lastDpad != 0x01) {
            escSpeed += ESC_STEP;
            escSpeed = constrain(escSpeed, ESC_PPM, 70);

            Serial.printf("Velocidade: %d\n", escSpeed);

            myGamepad->setColorLED(255, 150, 0); 
            delay(200);
            myGamepad->setColorLED(0, 255, 0);
            myGamepad->playDualRumble(0, 250, 0x80,0x40);
        }

        // DPad DOWN -> volta para velocidade mínima
        if (dpad == 0x02 && lastDpad != 0x02) {
            escSpeed = ESC_PPM;

            Serial.println("Velocidade resetada");

            myGamepad->setColorLED(255, 150, 0); 
            delay(200);  
            myGamepad->setColorLED(0, 255, 0);
            myGamepad->playDualRumble(0, 250, 0x80,0x40);
        }

        lastDpad = dpad;

        /*
        // ===========================================================
        // DEBUG PRINT — PRINTA TUDO NO FIM DO LOOP
        // ===========================================================
        Serial.println("=================================================");
        Serial.println("ESTADO DO CONTROLE PS4");
        Serial.println("-------------------------------------------------");


        Serial.println("-------------------------------------------------");
        Serial.printf("Botoes: X=%d  O=%d  R1=%d  L1=%d  R2=%d dpad=%d\n",
                      myGamepad->a(),
                      myGamepad->b(),
                      myGamepad->r1(),
                      myGamepad->l1(),
                      myGamepad->throttle(),
                      myGamepad->dpad());

        Serial.println("=================================================\n");
        */
    }

    delay(10);  // ajustável
}