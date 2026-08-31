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
bool escSpeedChanged = false;// flag para mudança de velocidade
const int ESC_STEP = 1;      // passo de aumento
uint16_t lastDpad = 0;       // guarda estado anterior do DPad

// Freio
const int BRK_ON  = 45;
const int BRK_OFF = 141;

// Marcha
const int GEAR_REVERSE = 60;
const int GEAR_FORWARD = 200;

// Pinos Auxiliares
#define buzzer 21
#define led_azul 25
#define led_verde 27
#define led_vermelho 26
#define btn_pareamento 16  // Botão físico de pareamento

// ======================================================
// VARIÁVEIS DE ESTADO
// ======================================================
bool gearForward = true;
bool lastR1 = false;
uint32_t lastBtnPress = 0; // Para o debounce do botão físico

// Flag volátil para a interrupção externa
volatile bool flagPareamento = false;

// Ponteiro para o controle
GamepadPtr myGamepad;


// ======================================================
// INTERRUPÇÃO EXTERNA (ISR)
// ======================================================
// O modificador IRAM_ATTR é obrigatório no ESP32 para carregar a função na RAM
void IRAM_ATTR isrBotaoPareamento() {
    flagPareamento = true; // Apenas levanta a bandeira para o loop processar
}


// ======================================================
// FUNÇÕES AUXILIARES (LED E BUZZER)
// ======================================================

// Controla o LED do Gamepad e o LED RGB físico simultaneamente
void setMyLeds(uint8_t r, uint8_t g, uint8_t b, GamepadPtr gp = nullptr) {
    // Aplica no LED RGB físico da placa
    analogWrite(led_vermelho, r);
    analogWrite(led_verde, g);
    analogWrite(led_azul, b);

    // Aplica no controle (se passado por parâmetro ou se já estiver pareado)
    if (gp != nullptr) {
        gp->setColorLED(r, g, b);
    } else if (myGamepad && myGamepad->isConnected()) {
        myGamepad->setColorLED(r, g, b);
    }
}

// Função para gerar um bipe rápido
void buzzerBeep(int time_ms) {
    digitalWrite(buzzer, HIGH);
    delay(time_ms);
    digitalWrite(buzzer, LOW);
}


// ======================================================
// BLUEPAD — CALLBACKS
// ======================================================
void onConnectedGamepad(GamepadPtr gp) {
    myGamepad = gp;
    setMyLeds(0, 255, 0, gp); // Verde ao conectar
    buzzerBeep(150); // Bipe ao conectar
    Serial.println("Controle Conectado!");
}

void onDisconnectedGamepad(GamepadPtr gp) {
    EscMtr.write(ESC_STOP);
    delay(200);
    SrvBrk.write(BRK_ON);
    
    setMyLeds(0, 0, 0, gp); // Apaga LEDs
    buzzerBeep(500); // Bipe longo ao perder conexão
    Serial.println("Controle Desconectado!");
    
    if (myGamepad == gp) {
        myGamepad = nullptr;
    }
}


// ======================================================
// SETUP
// ======================================================
void setup() {
    // 1) GARANTE QUE O BUZZER INICIE EM LOW
    pinMode(buzzer, OUTPUT);
    digitalWrite(buzzer, LOW);

    // 2) CONFIGURA O BOTÃO COMO INTERRUPÇÃO (Aciona na descida do sinal: HIGH -> LOW)
    pinMode(btn_pareamento, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(btn_pareamento), isrBotaoPareamento, FALLING);

    // Configura Pinos dos LEDs
    pinMode(led_vermelho, OUTPUT);
    pinMode(led_verde, OUTPUT);
    pinMode(led_azul, OUTPUT);
    setMyLeds(0, 0, 0); // Começa apagado

    Serial.begin(115200);

    // Servos
    SrvDir.attach(19);
    EscMtr.attach(32);
    SrvBrk.attach(18);
    SrvGear.attach(17);

    // Calibração do ESC
    EscMtr.write(ESC_PPM);
    delay(200);
    EscMtr.write(ESC_STOP);

    // Pisca os LEDs informando que o ESC terminou de calibrar
    for (int i = 0; i < 2; i++) {
        setMyLeds(255, 0, 0); delay(150); // Vermelho
        setMyLeds(0, 255, 0); delay(150); // Verde
        setMyLeds(0, 0, 255); delay(150); // Azul
    }
    setMyLeds(0, 0, 0); // Apaga após animação
    buzzerBeep(100); // Bipe indicando que está pronto

    // Valores iniciais
    SrvDir.write(DIR_CENTER);
    SrvBrk.write(BRK_OFF);
    SrvGear.write(GEAR_FORWARD);

    // Bluepad
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    BP32.enableVirtualDevice(false);

    Serial.println("Sistema iniciado! Aguardando conexao...");
}


// ======================================================
// FUNÇÃO PRINCIPAL DO LOOP
// ======================================================
void loop() {
    BP32.update();

    // ==================================================
    // LÓGICA DA INTERRUPÇÃO DO BOTÃO FÍSICO
    // ==================================================
    if (flagPareamento) {
        flagPareamento = false; // Reseta a flag
        
        // Debounce de 1 segundo para ignorar múltiplos toques rápidos acidentais (ruído mecânico)
        if (millis() - lastBtnPress > 1000) {
            lastBtnPress = millis();
            
            Serial.println("Botao fisico pressionado (Interrupcao)! Apagando chaves Bluetooth...");
            
            // Feedback sonoro e visual de pareamento
            buzzerBeep(150); delay(100); buzzerBeep(150);
            setMyLeds(255, 255, 0); // Amarelo (indica modo de busca/pareamento)
            
            // Esquece os controles antigos
            BP32.forgetBluetoothKeys();

            // Se tiver um controle conectado agora, desconecta ele
            if (myGamepad && myGamepad->isConnected()) {
                myGamepad->disconnect();
            }
        }
    }


    // ==================================================
    // LÓGICA DO CONTROLE PS4
    // ==================================================
    if (myGamepad && myGamepad->isConnected()) {

        // ================================
        // 1) DIREÇÃO — ANALÓGICO ESQUERDO
        // ================================
        int lx = myGamepad->axisX();  
        int dirValue = map(lx, -511, 511, DIR_LEFT, DIR_RIGHT) + dirOffset;
        dirValue = constrain(dirValue, DIR_LEFT, DIR_RIGHT);  
        SrvDir.write(dirValue);

        // ================================
        // 2) ACELERAÇÃO — BOTÃO X
        // ================================
        bool xPressed = myGamepad->a(); 

        if (xPressed || escSpeedChanged) {
            escSpeedChanged = false;
            SrvBrk.write(BRK_OFF);
            delay(200);
            EscMtr.write(escSpeed);
            setMyLeds(0, 0, 255); // Fica Azul enquanto acelera
        }

        // ================================
        // 3) FREIO — BOTÃO O
        // ================================
        bool oPressed = myGamepad->b();

        if (oPressed) {
            EscMtr.write(ESC_STOP);
            delay(100);
            SrvBrk.write(BRK_ON);
            
            setMyLeds(255, 0, 0); // Vermelho para freio
            myGamepad->playDualRumble(0, 250, 0x80, 0x40);
            
            delay(500);
            SrvBrk.write(BRK_OFF);
            setMyLeds(0, 255, 0); // Volta pro Verde (normal)
        }

        // ================================
        // 4) MARCHA — R1 toggle
        // ================================
        bool r1Pressed = myGamepad->r1();

        if (r1Pressed && !lastR1) {
            gearForward = !gearForward;
            if (gearForward) {
                SrvGear.write(GEAR_FORWARD);
                buzzerBeep(100); // 1 bipe = Frente
            } else {
                SrvGear.write(GEAR_REVERSE);
                buzzerBeep(80); delay(80); buzzerBeep(80); // 2 bipes curtos = Ré
            }
        }
        lastR1 = r1Pressed;


        // ================================
        // AJUSTE FINO DA DIREÇÃO — SETAS
        // ================================
        uint16_t dpad  = myGamepad->dpad();
 
        // Esquerda
        if (dpad == 0x08) {
            dirOffset -= DIR_OFFSET_STEP;
            setMyLeds(255, 150, 0); // Laranja
            buzzerBeep(50);
            delay(150);
            setMyLeds(0, 255, 0); // Volta para verde
            myGamepad->playDualRumble(0, 250, 0x80, 0x40);
        }

        // Direita
        if (dpad == 0x04) {
            dirOffset += DIR_OFFSET_STEP;
            setMyLeds(0, 150, 255); // Ciano
            buzzerBeep(50);
            delay(150); 
            setMyLeds(0, 255, 0); // Volta para verde
            myGamepad->playDualRumble(0, 250, 0x80, 0x40);
        }

        // ================================
        // CONTROLE DE VELOCIDADE
        // ================================

        // DPad UP -> aumenta velocidade
        if (dpad == 0x01 && lastDpad != 0x01) {
            escSpeed += ESC_STEP;
            escSpeed = constrain(escSpeed, ESC_PPM, 70);

            Serial.printf("Velocidade: %d\n", escSpeed);

            setMyLeds(255, 150, 0); 
            buzzerBeep(100);
            delay(100);
            setMyLeds(0, 255, 0);
            
            myGamepad->playDualRumble(0, 250, 0x80, 0x40);
            escSpeedChanged = true;
        }

        // DPad DOWN -> volta para velocidade mínima
        if (dpad == 0x02 && lastDpad != 0x02) {
            escSpeed = ESC_PPM;

            Serial.println("Velocidade resetada");

            setMyLeds(255, 0, 255); // Roxo quando reseta velocidade
            buzzerBeep(200);
            delay(100);  
            setMyLeds(0, 255, 0);
            
            myGamepad->playDualRumble(0, 250, 0x80, 0x40);
            escSpeedChanged = true;
        }

        lastDpad = dpad;
    }

    delay(10);
}