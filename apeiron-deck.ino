#include <Joystick.h>
#include <Mouse.h>

/* =================================================================
 * V7.0 - CACHYOS / LINUX ÖZEL CONFIG
 * =================================================================
 * NOT: Linux "js" (Joystick) sürücüleri tuşları 0'dan başlatır.
 * Standart Linux Gamepad Sıralaması Genelde Şöyledir:
 * 0:A, 1:B, 2:X, 3:Y, 4:LB, 5:RB, 6:Back, 7:Start, 8:Guide, 9:L3, 10:R3
 */

// --- 1. TUŞ NUMARALARI (ID) AYARLARI ---
// Eğer test ekranında "A" tuşu "B" olarak görünüyorsa, buradaki sayıları değiştir.

int ID_A = 0;       // Linux'ta A
int ID_B = 1;       // Linux'ta B
int ID_X = 4;       // Linux'ta X
int ID_Y = 3;       // Linux'ta Y
int ID_LB = 2;      // Linux'ta LB
int ID_RB = 5;      // Linux'ta RB
int ID_BACK = 12;    // Back (Select)
int ID_START = 11;   // Start
int ID_GUIDE = 6;   // Guide (Xbox Tuşu) - L3 buraya karışıyordu, ayırdık.
int ID_L3 = 7;      // Sol Joystick Butonu (Thumb L)
int ID_R3 = 10;     // Sağ Joystick Butonu (Thumb R)

// Yön Tuşları ve Tetikler (Bunlar bazen Eksen, bazen Buton olur)
int ID_UP = 13;     
int ID_DOWN = 8;
int ID_LEFT = 9;
int ID_RIGHT = 14;
int ID_LT = 15;     
int ID_RT = 16;     // Toplam 17 Buton tanımlayacağız (0-16)

// --- 2. PIN AYARLARI (SENİN DEVRE ŞEMAN) ---
// Kabloların nereye lehimli olduğunu buradan teyit et.
#define PIN_A 2
#define PIN_B 3
#define PIN_X 5
#define PIN_Y 4
#define PIN_RB 6
#define PIN_DOWN 7
#define PIN_RIGHT 8
#define PIN_LEFT 9
#define PIN_UP 10
#define PIN_LB 11
#define PIN_LT 12
#define PIN_RT 13
// A5 Pini: L3 (Direkt) ve R3 (Dirençli) okur.

// --- 3. JOYSTICK YÖN AYARLARI ---
bool invertLeftX  = true;  
bool invertLeftY  = true;  
bool invertRightX = false;
bool invertRightY = true;  

// --- BATARYA VE DEĞİŞKENLER ---
float batteryR1 = 100000.0;
float batteryR2 = 20000.0;
unsigned long lastBatteryUpdate = 0;
bool mouseModeActive = false;
int mouseSpeed = 500;    
int deadzone = 5;      
unsigned long r3PressStartTime = 0; 
bool r3Held = false;

// Joystick Tanımlama (17 Buton)
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
  17, 0,                  // 17 Buton
  true, true, false,      // X, Y (Var)
  true, true, false,      // Rx, Ry (Var)
  false, false,           // Rudder/Throttle KAPALI (Axis 6 sorununu çözer)
  false, false, false);   

void setup() {
  Serial.begin(9600); 

  // Pinleri Giriş Yap
  for (int i = 2; i <= 13; i++) pinMode(i, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);

  Joystick.begin();
  Mouse.begin();
  
  Joystick.setXAxisRange(0, 1023);
  Joystick.setYAxisRange(0, 1023);
  Joystick.setRxAxisRange(0, 1023);
  Joystick.setRyAxisRange(0, 1023);
}

void loop() {
  // -----------------------------------------------------------
  // 1. TUŞ OKUMALARI
  // -----------------------------------------------------------
  
  // A5 Pini (L3 ve R3)
  int a5Value = analogRead(A5);
  bool btnL3 = (a5Value < 100);                    // Dirençsiz (L3)
  bool btnR3 = (a5Value > 200 && a5Value < 900);   // Dirençli (R3 - Mod Tuşu)

  // Dijital Pinler (!digitalRead çünkü Pullup var)
  bool bA = !digitalRead(PIN_A);
  bool bB = !digitalRead(PIN_B);
  bool bX = !digitalRead(PIN_X);
  bool bY = !digitalRead(PIN_Y);
  bool bLB = !digitalRead(PIN_LB);
  bool bRB = !digitalRead(PIN_RB);
  bool bLT = !digitalRead(PIN_LT);
  bool bRT = !digitalRead(PIN_RT);
  bool bUp = !digitalRead(PIN_UP);
  bool bDown = !digitalRead(PIN_DOWN);
  bool bLeft = !digitalRead(PIN_LEFT);
  bool bRight = !digitalRead(PIN_RIGHT);

  // --- DONANIMSAL KISA DEVRE FİLTRESİ ---
  // "Triggerlara basınca RB basıyor" sorunu için yazılımsal yama.
  // Eğer LT veya RT basılıysa, RB sinyalini yoksay (Kablolar temas ediyorsa işe yarar)
  if (bLT || bRT) {
     bRB = false; // Trigger basılıyken RB'yi iptal et
  }

  // -----------------------------------------------------------
  // 2. MOD DEĞİŞTİRME (R3'e BASILI TUTMA)
  // -----------------------------------------------------------
  if (btnR3) {
    if (r3PressStartTime == 0) r3PressStartTime = millis();
    if (millis() - r3PressStartTime > 2000) { 
      if (!r3Held) {
        mouseModeActive = !mouseModeActive; 
        r3Held = true; 
        if (mouseModeActive) Serial.println("SYS:MOUSE_ON"); 
        else Serial.println("SYS:MOUSE_OFF"); 
        TXLED1; delay(100); TXLED0; delay(100); TXLED1; delay(100); TXLED0; 
      }
    }
  } else {
    r3PressStartTime = 0;
    r3Held = false;
  }

  // -----------------------------------------------------------
  // 3. ÇALIŞMA MODLARI
  // -----------------------------------------------------------
  
  if (mouseModeActive) {
    // --- MOUSE MODU ---
    if (!btnR3) { 
       int mX = analogRead(A1); // Rx
       int mY = analogRead(A2); // Ry
       
       if (invertRightX) mX = 1023 - mX;
       if (invertRightY) mY = 1023 - mY;

       if (abs(mX - 512) > deadzone) Mouse.move((mX - 512) / mouseSpeed, 0, 0);
       if (abs(mY - 512) > deadzone) Mouse.move(0, (mY - 512) / mouseSpeed, 0); 

       // Tıklamalar
       if (bRB) { if(!Mouse.isPressed(MOUSE_LEFT)) Mouse.press(MOUSE_LEFT); }
       else if(Mouse.isPressed(MOUSE_LEFT)) Mouse.release(MOUSE_LEFT);

       if (bLB) { if(!Mouse.isPressed(MOUSE_RIGHT)) Mouse.press(MOUSE_RIGHT); }
       else if(Mouse.isPressed(MOUSE_RIGHT)) Mouse.release(MOUSE_RIGHT);
    }
  } 
  else {
    // --- GAMEPAD MODU ---
    
    // Doğrudan Eşleştirme (Linux ID'lerine göre)
    Joystick.setButton(ID_A, bA);
    Joystick.setButton(ID_B, bB);
    Joystick.setButton(ID_X, bX);
    Joystick.setButton(ID_Y, bY);
    
    Joystick.setButton(ID_LB, bLB);
    Joystick.setButton(ID_RB, bRB);
    
    Joystick.setButton(ID_LT, bLT);
    Joystick.setButton(ID_RT, bRT);
    
    Joystick.setButton(ID_UP,    bUp);
    Joystick.setButton(ID_DOWN,  bDown);
    Joystick.setButton(ID_LEFT,  bLeft);
    Joystick.setButton(ID_RIGHT, bRight);
    
    // Joystick Butonları
    // "Guide olarak çalışıyor" dediğin L3'ü ID 9'a aldık, düzelmesi lazım.
    Joystick.setButton(ID_L3, btnL3); 
    
    // Start Tuşu:
    // Sağ ok start basıyor demiştin, burada "ID_START"ı kullanmıyoruz,
    // Start tuşun fiziken yok gibi? 
    // Eğer R3'ü (kısa basınca) START olarak kullanmak istiyorsan:
    if (!r3Held) Joystick.setButton(ID_START, btnR3); 
    else Joystick.setButton(ID_START, 0); 

    // ANALOGLAR
    int lx = analogRead(A3);
    int ly = analogRead(A4);
    int rx = analogRead(A1);
    int ry = analogRead(A2);

    if (invertLeftX) lx = 1023 - lx;
    if (invertLeftY) ly = 1023 - ly;
    if (invertRightX) rx = 1023 - rx;
    if (invertRightY) ry = 1023 - ry;

    Joystick.setXAxis(lx);
    Joystick.setYAxis(ly);
    Joystick.setRxAxis(rx);
    Joystick.setRyAxis(ry);
  }

  // --- BATARYA ---
  if (millis() - lastBatteryUpdate >= 1000) {
     long totalVal = 0; 
     for (int i=0; i<50; i++) { totalVal += analogRead(A0); delay(2); }
     float voltage = (totalVal / 50.0) * (5.0 / 1023.0) * ((batteryR1 + batteryR2) / batteryR2);
     int pct = map(voltage * 100, 1280, 1680, 0, 100);
     Serial.print("BAT:"); Serial.println(constrain(pct, 0, 100));
     lastBatteryUpdate = millis();
  }
}
