
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

#define BLYNK_PRINT Serial
#define SS_PIN D8
#define RST_PIN D4
#define BTN_PIN D2

MFRC522 mfrc522(SS_PIN, RST_PIN);
unsigned long uidDec, uidDecTemp;
int ARRAYindexUIDcard;
int EEPROMstartAddr;
long adminID = 1496524848;
bool beginCard = 0;
bool addCard = 1;
bool skipCard = 0;
int LockSwitch;
unsigned long Mem_Card[10];
int PiezoPin = D1;
int motor1 = D0;
int motor2 = D3;

int st1 = 0;
int st2 = 0;

char auth[] = "25682d933b9c4ed59f9d6b7a15eb2f7d";
char ssid[] = "GLHF";
char pass[] = "wakeru01";

WidgetLCD lcd(V0);

void setup() {
  Serial.begin(9600);
  pinMode(BUILTIN_LED,OUTPUT);
  pinMode(BTN_PIN, INPUT);
  pinMode(PiezoPin, OUTPUT);
  pinMode(motor1,OUTPUT);
  pinMode(motor2,OUTPUT);

  SPI.begin();
  mfrc522.PCD_Init();
  setup_wifi();

  Blynk.begin(auth, ssid, pass);
 
  lcd.clear();
  EEPROM.begin(512);
  DisplayWAiT_CARD();
  EEPROMreadUIDcard();
  digitalWrite(BUILTIN_LED, HIGH);
  digitalWrite(motor1,LOW);
  digitalWrite(motor2,LOW);
  analogWrite(PiezoPin, 255), delay(500), analogWrite(PiezoPin, 0);

}

void loop() {
  digitalWrite(BUILTIN_LED, LOW);
  digitalWrite(motor1,st1);
  digitalWrite(motor2,st2);


  if (!digitalRead(BTN_PIN) == HIGH) {
    digitalWrite(BUILTIN_LED, HIGH); //unlock
    lcd.print(0, 0, " BUTTON UNLOCK ");
    lcd.print(0, 1, "   DOOR OPEN   ");
    analogWrite(PiezoPin, 255), delay(700), analogWrite(PiezoPin, 0);
    motor_f(1710,2600);
    delay(2000);
    DisplayWAiT_CARD();
  }

  if (beginCard == 0) {
    if ( ! mfrc522.PICC_IsNewCardPresent()) { 
      Blynk.run();
      return;
    }

    if ( ! mfrc522.PICC_ReadCardSerial()) {
      Blynk.run();
      return;
    }
  }

  //Read "UID".
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidDecTemp = mfrc522.uid.uidByte[i];
    uidDec = uidDec * 256 + uidDecTemp;
  }

  if (beginCard == 1 || LockSwitch > 0)EEPROMwriteUIDcard();

  if (LockSwitch == 0) {
    //CardUIDeEPROMread.
    for (ARRAYindexUIDcard = 0; ARRAYindexUIDcard <= 9; ARRAYindexUIDcard++) {
      if (Mem_Card[ARRAYindexUIDcard] > 0) {
        if (Mem_Card[ARRAYindexUIDcard] == uidDec) {
          lcd.print(0, 0, "CARD ACCESS OPEN");
          lcd.print(3, 1, uidDec);
          digitalWrite(BUILTIN_LED, HIGH); //unlock
          analogWrite(PiezoPin, 255), delay(700), analogWrite(PiezoPin, 0);
          motor_f(1770,2550);
          break;
        }
      }
    }

    if (ARRAYindexUIDcard == 10) {
      lcd.print(0, 0, " Card not Found ");
      lcd.print(0, 1, "                ");
      lcd.print(0, 1, "ID : ");
      lcd.print(5, 1, uidDec);
      for (int i = 0; i <= 2; i++)delay(100), analogWrite(PiezoPin, 255), delay(500), analogWrite(PiezoPin, 0);
      digitalWrite(BUILTIN_LED, LOW);  //lock
      delay(2000);
    }

    ARRAYindexUIDcard = 0;
    DisplayWAiT_CARD();
  }

  Blynk.run();
}

BLYNK_WRITE(V1) {
  int a = param.asInt();
  if (a == 1) {
    beginCard = 1;
  }
  else {
  beginCard = 0;
  }
}

BLYNK_WRITE(V2) {
  int a = param.asInt();
  if (a == 1) {
    skipCard = 1;
    if (EEPROMstartAddr / 5 < 10) EEPROMwriteUIDcard();
  } else {
    skipCard = 0;
  }
}

BLYNK_WRITE(V3) {
  int a = param.asInt();
  if (a == 1) {
    digitalWrite(BUILTIN_LED, HIGH); //unlock
    lcd.print(0, 0, " APP UNLOCK OK ");
    lcd.print(0, 1, "   DOOR OPEN   ");
    analogWrite(PiezoPin, 255), delay(700), analogWrite(PiezoPin, 0);
    motor_f(1770,2720);
    delay(2000);
    DisplayWAiT_CARD();
  } 
}

BLYNK_WRITE(V4)
{ 
  st1 = param.asInt();
}

BLYNK_WRITE(V5)
{
  st2 = param.asInt(); 
}
void EEPROMwriteUIDcard() {
  if (LockSwitch == 0) {
    lcd.print(0, 0, " START REC CARD ");
    lcd.print(0, 1, "PLEASE TAG CARDS");
    delay(500);
  }

  if (LockSwitch > 0) {
    if (skipCard == 1) {
      lcd.print(0, 0, "   SKIP RECORD   ");
      lcd.print(0, 1, "                ");
      lcd.print(0, 1, "   label : ");
      lcd.print(11, 1, EEPROMstartAddr / 5);
      EEPROMstartAddr += 5;
      skipCard = 0;
    } else {
      Serial.println("writeCard");
      EEPROM.write(EEPROMstartAddr, uidDec & 0xFF);
      EEPROM.write(EEPROMstartAddr + 1, (uidDec & 0xFF00) >> 8);
      EEPROM.write(EEPROMstartAddr + 2, (uidDec & 0xFF0000) >> 16);
      EEPROM.write(EEPROMstartAddr + 3, (uidDec & 0xFF000000) >> 24);
      EEPROM.commit();
      delay(10);
      lcd.print(0, 1, "                ");
      lcd.print(0, 0, "RECORD OK! IN   ");
      lcd.print(0, 1, "MEMORY : ");
      lcd.print(9, 1, EEPROMstartAddr / 5);
      EEPROMstartAddr += 5;
      delay(500);
    }
  }

  LockSwitch++;

  if (EEPROMstartAddr / 5 == 10) {
    lcd.clear();
    lcd.print(0, 0, "RECORD FINISH");
    delay(2000);
    EEPROMstartAddr = 0;
    uidDec = 0;
    ARRAYindexUIDcard = 0;
    EEPROMreadUIDcard();
  }
}

void EEPROMreadUIDcard() {
  for (int i = 0; i < 10; i++) {
    byte val = EEPROM.read(EEPROMstartAddr + 3);
    Mem_Card[ARRAYindexUIDcard] = (Mem_Card[ARRAYindexUIDcard] << 8) | val;
    val = EEPROM.read(EEPROMstartAddr + 2);
    Mem_Card[ARRAYindexUIDcard] = (Mem_Card[ARRAYindexUIDcard] << 8) | val;
    val = EEPROM.read(EEPROMstartAddr + 1);
    Mem_Card[ARRAYindexUIDcard] = (Mem_Card[ARRAYindexUIDcard] << 8) | val;
    val = EEPROM.read(EEPROMstartAddr);
    Mem_Card[ARRAYindexUIDcard] = (Mem_Card[ARRAYindexUIDcard] << 8) | val;

    ARRAYindexUIDcard++;
    EEPROMstartAddr += 5;
  }

  ARRAYindexUIDcard = 0;
  EEPROMstartAddr = 0;
  uidDec = 0;
  LockSwitch = 0;
  DisplayWAiT_CARD();
}

void DisplayWAiT_CARD() {
  lcd.print(0, 0, "   ATTACH THE   ");
  lcd.print(0, 1, "      CARD      ");
}

BLYNK_WRITE(V6){
  for(int x=0 ; x<10; x++){
    Mem_Card[x] = x;
  }
  lcd.print(0, 0, " DELETE All ID "); 
  lcd.print(0, 1, "   COMPLETED.   ");
  analogWrite(PiezoPin, 10), delay(400), analogWrite(PiezoPin, 0);
  delay(1000);
  DisplayWAiT_CARD();
}

void motor_f(int d1,int d2){
  Blynk.run();
  lcd.print(0, 0, " PLEASE..WAIT ! "), lcd.print(0, 1, "  OPENING..... ");
  digitalWrite(motor1,HIGH);
  digitalWrite(motor2,LOW);
  delay(d1);
  digitalWrite(motor1,LOW);
  digitalWrite(motor2,LOW);
  lcd.print(0, 0, "THE DOOR OPENED. "), lcd.print(0, 1, "  5 SECOND..");
  delay(5000);
  lcd.print(0, 0, " THE DOOR IS     "), lcd.print(0, 1, "  CLOSING...");
  digitalWrite(motor1,LOW);
  digitalWrite(motor2,HIGH);
  delay(d2);
  digitalWrite(motor1,LOW);
  digitalWrite(motor2,LOW);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.print(0, 0, " PLASE..WAIT "), lcd.print(0, 1, " CONNECTING..WIFI ");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
