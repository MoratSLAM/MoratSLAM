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
int dirOffset = 0;     // Ajuste fino da direção (positivo=>direita / negativo=>esquerda)
const int DIR_OFFSET_STEP = 1;   // tamanho do passo por clique

// ESC
const int ESC_MIN = 65;
const int ESC_MAX = 80;
const int ESC_STOP = 20;
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

bool lastL1 = false;
bool lastR1 = false;

bool accelEnabled = false;
bool lastX = false;

// Ponteiro para o controle
GamepadPtr myGamepad;

int escValue = 0;

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
        // 2) ACELERAÇÃO — BOTÃO X (toggle)
        // ================================
        bool xPressed = myGamepad->a(); 

        if (xPressed && !lastX) {
            accelEnabled = !accelEnabled;

            if (accelEnabled) {
                //
                // ---- ACELERAR ----
                //
                SrvBrk.write(BRK_OFF);
                delay(200);
                EscMtr.write(ESC_MIN);
                myGamepad->setColorLED(0, 0, 255);
            }
            else {
                //
                // ---- PARAR ----
                //
                EscMtr.write(ESC_STOP);
                delay(150);
                SrvBrk.write(BRK_ON);
                myGamepad->setColorLED(255, 0, 0);
                delay(200);
            }
        }
        lastX = xPressed;

        // Segurança: se freio do L1 estiver ativo, motor sempre para
        if (brakeEnabled) {
            EscMtr.write(ESC_STOP);
            SrvBrk.write(BRK_ON);
        }

        // ================================
        // 3) FREIO — L1 toggle
        // ================================
        bool l1Pressed = myGamepad->l1();

        if (l1Pressed && !lastL1) {
            brakeEnabled = !brakeEnabled;
            if (brakeEnabled) {
                EscMtr.write(ESC_STOP);
                delay(100);
                SrvBrk.write(BRK_ON);
                myGamepad->setColorLED(255, 0, 0);
                myGamepad->playDualRumble(0, 250, 0x80, 0x40);
            }
            else {
                SrvBrk.write(BRK_OFF);
                myGamepad->setColorLED(0, 255, 0);
            }
        }
        lastL1 = l1Pressed;

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

        // ================================
        // AJUSTE FINO DA DIREÇÃO — SETAS
        // ================================
        uint16_t dpad  = myGamepad->dpad();
 
        if (dpad == 0x08) {
            dirOffset -= DIR_OFFSET_STEP;
            myGamepad->setColorLED(255, 150, 0);     // LED laranja ao ajustar
            delay(200);  // evita repetir muito rápido
            myGamepad->setColorLED(0, 255, 0);
            myGamepad->playDualRumble(0, 250, 0x80,0x40);
        }

        if (dpad == 0x04) {
            dirOffset += DIR_OFFSET_STEP;
            myGamepad->setColorLED(0, 150, 255);     // LED azul-claro ao ajustar
            delay(200); // evita repetir muito rápido
            myGamepad->setColorLED(0, 255, 0);
            myGamepad->playDualRumble(0, 250, 0x80,0x40);
        }

        if (dpad == 0x02) {
            dirOffset = 0;
            myGamepad->setColorLED(255, 0, 0);     // LED azul-claro ao ajustar
            delay(200); // evita repetir muito rápido
            myGamepad->setColorLED(0, 255, 0);
            myGamepad->playDualRumble(0, 250, 0x80,0x40);
        }
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
