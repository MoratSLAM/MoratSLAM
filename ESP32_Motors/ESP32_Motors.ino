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

// ESC
const int ESC_MIN = 65;
const int ESC_MAX = 80;
const int ESC_STOP = 0;
bool ESC_FLAG = false;

// Freio
const int BRK_ON  = 41;
const int BRK_OFF = 141;

// Marcha
const int GEAR_REVERSE = 60;
const int GEAR_FORWARD = 200;


// ======================================================
// VARIÁVEIS DE ESTADO
// ======================================================
bool brakeEnabled = false;
bool gearForward = true;

// Para toggle (evitar repetir várias vezes no mesmo clique)
bool lastL2 = false;
bool lastR1 = false;

// Ponteiro para o controle
GamepadPtr myGamepad;


// ======================================================
// BLUEPAD — CALLBACKS
// ======================================================
void onConnectedGamepad(GamepadPtr gp) {
    myGamepad = gp;
    Serial.println("Controle conectado!");
    myGamepad->setColorLED(0, 255, 0);
    
}

void onDisconnectedGamepad(GamepadPtr gp) {
    if (myGamepad == gp) {
        myGamepad = nullptr;
    }
    Serial.println("Controle desconectado");
    EscMtr.write(ESC_STOP);
}


// ======================================================
// SETUP
// ======================================================
void setup() {
    Serial.begin(115200);

    // Servos
    SrvDir.attach(19);
    EscMtr.attach(16);
    SrvBrk.attach(17);
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


// ======================================================
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
        int dirValue = map(lx, -511, 511, DIR_LEFT, DIR_RIGHT);
        SrvDir.write(dirValue);

        // ================================
        // 2) ACELERAÇÃO — ANALÓGICO DIREITO
        // ================================
        // gp->axisRY vai de -511 (baixo) a 511 (cima)
        int ry = myGamepad->axisRY();

        if (ry > -20) {
            // Analogico solto → motor parado
            EscMtr.write(ESC_STOP);
        } else {
            int escValue = map(ry, 0, -508, ESC_MIN, ESC_MAX);
            if(!brakeEnabled){
                EscMtr.write(escValue);
            }
        }

        // ================================
        // 3) FREIO — L2 toggle
        // ================================
        int l2 = myGamepad->brake();
        bool l2Pressed = l2 > 200;

        if (l2Pressed && !lastL2) {
            brakeEnabled = !brakeEnabled;
            if (brakeEnabled){
                EscMtr.write(ESC_STOP);
                SrvBrk.write(BRK_ON);
                myGamepad->setColorLED(255, 0, 0);
                myGamepad->playDualRumble(0, 250, 0x80,0x40);
            }
            else{
                SrvBrk.write(BRK_OFF);
                myGamepad->setColorLED(0, 255, 0);
            }
        }
        lastL2 = l2Pressed;

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
        // 5) BOTÃO OPTIONS — AÇÃO ESPECIAL
        // ================================
        uint16_t psPressed = myGamepad->miscButtons();

        if (psPressed == 0x04 && ESC_FLAG == false) {
            configESC();
            psPressed == 0x00;
        } 
        else if (psPressed == 0x04 && ESC_FLAG == true){
            myGamepad->playDualRumble(0, 250, 0x80,0x40);
            myGamepad->setColorLED(255, 0, 0);
            delay(500);
            myGamepad->setColorLED(0, 255, 0);
            myGamepad->playDualRumble(0, 250, 0x80,0x40);
            delay(500);
            myGamepad->setColorLED(255, 0, 0);
            myGamepad->playDualRumble(0, 250, 0x80,0x40);
            delay(500);
            myGamepad->setColorLED(0, 255, 0);
            psPressed == 0x00;
        }

        
        /*
        // ===========================================================
        // DEBUG PRINT — PRINTA TUDO NO FIM DO LOOP
        // ===========================================================
        Serial.println("=================================================");
        Serial.println("ESTADO DO CONTROLE PS4");
        Serial.println("-------------------------------------------------");

        Serial.printf("LX (direcao): %d  -> servo = %d\n", lx, dirValue);

        Serial.printf("R2 (aceleracao): %d  -> ESC = %d\n", r2, escValue);

        Serial.printf("L2 (freio): %d  -> estado = %s\n",
                      l2,
                      brakeEnabled ? "FREANDO" : "SOLTO");

        Serial.printf("R1 (marcha): %s\n",
                      gearForward ? "FRENTE" : "RÉ");

        Serial.println("-------------------------------------------------");
        Serial.printf("Botoes: X=%d  O=%d  R1=%d  L1=%d  R2=%d  L2=%d\n",
                      myGamepad->a(),
                      myGamepad->b(),
                      myGamepad->r1(),
                      myGamepad->l1(),
                      myGamepad->throttle(),
                      myGamepad->brake());

        Serial.println("=================================================\n");
        */
    }

    delay(10);  // ajustável
}

void configESC()
{
  // Início da configuração do ESC
  myGamepad->setColorLED(0, 0, 255);

  EscMtr.write(450);
  // Aguarda 35,8 segundos para chegar ao modo default do ESC
  delay(35800);
  
  // Descida de valores até 0 para configurar o ESC
  for (int i = 450; i >= 0; i--)
  { 
    EscMtr.write(i);
    delay(5);
  }
  delay(5000);

  // Fim da configuração do ESC
  myGamepad->setColorLED(0, 255, 0);
  ESC_FLAG = true;
}
