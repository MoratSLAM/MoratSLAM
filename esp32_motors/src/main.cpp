#include <Arduino.h>
#include <ESP32Servo.h>
#include <Bluepad32.h>

// ======================================================
// SERVOS AND HARDWARE CONSTANTS
// ======================================================
Servo SrvDir;    // steering
Servo EscMtr;    // motor
Servo SrvBrk;    // brake
Servo SrvGear;   // gear

// Steering
const int DIR_LEFT  = 41;
const int DIR_RIGHT = 141;
const int DIR_CENTER = 92;
int dirOffset = 13;     // Fine steering adjustment (positive=>right / negative=>left)
const int DIR_OFFSET_STEP = 1;   // adjustment step

// ESC
const int ESC_PPM = 66;
const int ESC_STOP = 20;
int escSpeed = ESC_PPM;      // current speed
bool escSpeedChanged = false;// speed change flag
const int ESC_STEP = 1;      // increase step
uint16_t lastDpad = 0;       // stores the previous DPad state

// Brake
const int BRK_ON  = 45;
const int BRK_OFF = 141;

// Gear
const int GEAR_REVERSE = 60;
const int GEAR_FORWARD = 200;

// Auxiliary pins
#define buzzer 21
#define led_azul 26
#define led_verde 27
#define led_vermelho 25
#define btn_pareamento 16  // Physical pairing button

// ======================================================
// STATE VARIABLES
// ======================================================
bool gearForward = true;
bool lastR1 = false;
bool robotRunning = false; 
uint32_t lastBtnPress = 0; // For physical button debouncing

// Volatile flag for the external interrupt
volatile bool flagPareamento = false;

// Gamepad pointer
GamepadPtr myGamepad;


// ======================================================
// EXTERNAL INTERRUPT (ISR)
// ======================================================
void IRAM_ATTR isrBotaoPareamento() {
    flagPareamento = true; // Only raise the flag for the loop to process
}


// ======================================================
// HELPER FUNCTIONS (LED AND BUZZER)
// ======================================================

// Control the gamepad LED and the physical RGB LED simultaneously
void setMyLeds(uint8_t r, uint8_t g, uint8_t b, GamepadPtr gp = nullptr) {
    // Apply to the board's physical RGB LED
    analogWrite(led_vermelho, r);
    analogWrite(led_verde, g);
    analogWrite(led_azul, b);

    // Apply to the gamepad (if passed as a parameter or already paired)
    if (gp != nullptr) {
        gp->setColorLED(r, g, b);
    } else if (myGamepad && myGamepad->isConnected()) {
        myGamepad->setColorLED(r, g, b);
    }
}

// Function to generate a quick beep
void buzzerBeep(int time_ms) {
    digitalWrite(buzzer, HIGH);
    delay(time_ms);
    digitalWrite(buzzer, LOW);
}


// ======================================================
// BLUEPAD - CALLBACKS
// ======================================================
void onConnectedGamepad(GamepadPtr gp) {
    myGamepad = gp;
    setMyLeds(0, 255, 0, gp); // Green when connected
    buzzerBeep(150); // Beep when connected
}

void onDisconnectedGamepad(GamepadPtr gp) {
    EscMtr.write(ESC_STOP);
    delay(200);
    SrvBrk.write(BRK_ON);
    
    setMyLeds(0, 0, 0, gp); // Turn off LEDs
    buzzerBeep(500);
    delay(200);
    buzzerBeep(500);
    delay(200);
    buzzerBeep(500);
    
    if (myGamepad == gp) {
        myGamepad = nullptr;
    }
}


// ======================================================
// SETUP
// ======================================================
void setup() {
    // ENSURE THE BUZZER STARTS LOW
    pinMode(buzzer, OUTPUT);
    digitalWrite(buzzer, LOW);

    // CONFIGURE THE BUTTON AS AN INTERRUPT (Triggers on signal falling edge)
    pinMode(btn_pareamento, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(btn_pareamento), isrBotaoPareamento, FALLING);

    // Configure LED pins
    pinMode(led_vermelho, OUTPUT);
    pinMode(led_verde, OUTPUT);
    pinMode(led_azul, OUTPUT);
    setMyLeds(0, 0, 0); // Start turned off

    Serial.begin(115200);

    // Servos
    SrvDir.attach(19);
    EscMtr.attach(32);
    SrvBrk.attach(18);
    SrvGear.attach(17);

    // ESC calibration
    EscMtr.write(ESC_PPM);
    delay(200);
    EscMtr.write(ESC_STOP);

    // Flash the LEDs to indicate that ESC calibration is complete
    for (int i = 0; i < 2; i++) {
        setMyLeds(255, 0, 0); delay(150); // Red
        setMyLeds(0, 255, 0); delay(150); // Green
        setMyLeds(0, 0, 255); delay(150); // Blue
    }
    setMyLeds(0, 0, 0); // Turn off after animation
    buzzerBeep(100); // Beep indicating readiness

    // Initial values
    SrvDir.write(DIR_CENTER);
    SrvBrk.write(BRK_OFF);
    SrvGear.write(GEAR_FORWARD);

    // Bluepad
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    BP32.enableVirtualDevice(false);

}


// ======================================================
// MAIN LOOP FUNCTION
// ======================================================
void loop() {
    BP32.update();

    // ==================================================
    // PHYSICAL BUTTON INTERRUPT LOGIC
    // ==================================================
    if (flagPareamento) {
        flagPareamento = false; // Reset the flag
        
        // One-second debounce to ignore multiple accidental quick presses
        if (millis() - lastBtnPress > 1000) {
            lastBtnPress = millis();
            
            // Audio and visual pairing feedback
            buzzerBeep(150); delay(100); buzzerBeep(150);
            setMyLeds(255, 255, 0); // Yellow (indicates search/pairing mode)
            
            // Forget old controllers
            BP32.forgetBluetoothKeys();

            // If a controller is currently connected, disconnect it
            if (myGamepad && myGamepad->isConnected()) {
                myGamepad->disconnect();
            }
        }
    }


    // ==================================================
    // PS4 CONTROLLER LOGIC
    // ==================================================
    if (myGamepad && myGamepad->isConnected()) {

        // ================================
        // STEERING - LEFT ANALOG STICK
        // ================================
        int lx = myGamepad->axisX();  
        int dirValue = map(lx, -511, 511, DIR_LEFT, DIR_RIGHT) + dirOffset;
        dirValue = constrain(dirValue, DIR_LEFT, DIR_RIGHT);  
        SrvDir.write(dirValue);

        // ================================
        // ACCELERATION - X BUTTON
        // ================================
        bool xPressed = myGamepad->a(); 

        if (xPressed) {
            if (!robotRunning) {
                SrvBrk.write(BRK_OFF);
                delay(200);
                robotRunning = true;
            }
            escSpeedChanged = false;
            EscMtr.write(escSpeed);
            setMyLeds(0, 0, 255); // Blue while accelerating
        }

        // ================================
        // BRAKE - O BUTTON
        // ================================
        bool oPressed = myGamepad->b();

        if (oPressed) {
            robotRunning = false;
            EscMtr.write(ESC_STOP);
            delay(100);
            SrvBrk.write(BRK_ON);
            
            setMyLeds(255, 0, 0); // Red for braking
            myGamepad->playDualRumble(0, 250, 0x80, 0x40);
            
            delay(500);
            SrvBrk.write(BRK_OFF);
            setMyLeds(0, 255, 0); // Return to green
        }

        // ================================
        // GEAR - R1 toggle
        // ================================
        bool r1Pressed = myGamepad->r1();

        if (r1Pressed && !lastR1 && !robotRunning) { 
            gearForward = !gearForward;
            if (gearForward) {
                SrvGear.write(GEAR_FORWARD);
                buzzerBeep(100); // 1 beep = forward
            } else {
                SrvGear.write(GEAR_REVERSE);
                buzzerBeep(80); delay(80); buzzerBeep(80); // 2 short beeps = reverse
            }
        }
        lastR1 = r1Pressed;


        // ================================
        // FINE STEERING ADJUSTMENT - D-PAD
        // ================================
        uint16_t dpad  = myGamepad->dpad();
 
        // Left
        if (dpad == 0x08) {
            dirOffset -= DIR_OFFSET_STEP;
            setMyLeds(255, 150, 0); // Orange
            buzzerBeep(50);
            delay(150);
            setMyLeds(0, 255, 0); // Return to green
            myGamepad->playDualRumble(0, 250, 0x80, 0x40);
        }

        // Right
        if (dpad == 0x04) {
            dirOffset += DIR_OFFSET_STEP;
            setMyLeds(0, 150, 255); // Cyan
            buzzerBeep(50);
            delay(150); 
            setMyLeds(0, 255, 0); // Return to green
            myGamepad->playDualRumble(0, 250, 0x80, 0x40);
        }

        // ================================
        // SPEED CONTROL
        // ================================

        // DPad UP -> increase speed
        if (dpad == 0x01 && lastDpad != 0x01) {
            escSpeed += ESC_STEP;
            escSpeed = constrain(escSpeed, ESC_PPM, 70);

            setMyLeds(255, 150, 0); 
            buzzerBeep(100);
            delay(100);
            setMyLeds(0, 255, 0);
            
            myGamepad->playDualRumble(0, 250, 0x80, 0x40);
            escSpeedChanged = true;
        }

        // DPad DOWN -> return to minimum speed
        if (dpad == 0x02 && lastDpad != 0x02) {
            escSpeed = ESC_PPM;

            setMyLeds(255, 0, 255); // Purple when resetting speed
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