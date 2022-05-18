#include <Firebase.h>
#include <FirebaseArduino.h>
#include <FirebaseCloudMessaging.h>
#include <FirebaseError.h>
#include <FirebaseHttpClient.h>
#include <FirebaseObject.h>

#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <FirebaseArduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

#define WIFI_SSID "JITG Wi-Fi"
#define WIFI_PASSWORD "Comm$copew!re370"
//#define WIFI_SSID "HUAWEI_Y7_Pro_2018"

//#define WIFI_PASSWORD "123456789"

#define FIREBASE_HOST "smart-ac-controller-a373d.firebaseio.com"
#define FIREBASE_AUTH "tOZ07IUVHo0A2lU5uRORjuDTwRe46tig3WXm0HX7"

#define On_Board_LED 16

int power, fanSpd, swing, temp, acmode, jetCool, eSaving, clean;
int prePower = 1;
int preFanSpd = 3;
int preSwing = 0;
int preTemp = 24;
int preAcmode = 2;
int preJetCool = 0;
int preESaving = 0;
int preClean = 0;

////////////////////////////////////////////////

#include <IRremoteESP8266.h>
#include <IRsend.h>

IRsend irsend(4);  // An IR LED is controlled by GPIO pin 14 (D5)

// 0 : TOWER
// 1 : WALL
const unsigned int kAc_Type  = 1;

// 0 : cooling
// 1 : heating
unsigned int ac_heat = 1;

// 0 : off
// 1 : on
unsigned int ac_power_on = 0;

// 0 : off
// 1 : on --> power on
unsigned int ac_air_clean_state = 0;

// temperature : 18 ~ 30
unsigned int ac_temperature = 24;

// 0 : low
// 1 : mid
// 2 : high
// if kAc_Type = 1, 3 : change
unsigned int ac_flow = 1;

const uint8_t kAc_Flow_Tower[3] = {0, 4, 6};
const uint8_t kAc_Flow_Wall[4] = {0, 2, 4, 5};

uint32_t ac_code_to_sent;

////////////////////////////////////////////////////////////

void Ac_Send_Code(uint32_t code) {
  Serial.print("code to send : ");
  Serial.print(code, BIN);
  Serial.print(" : ");
  Serial.println(code, HEX);

#if SEND_LG
  irsend.sendLG(code, 28);
#else  // SEND_LG
  Serial.println("Can't send because SEND_LG has been disabled.");
#endif  // SEND_LG
}

void Ac_Activate(unsigned int temperature, unsigned int air_flow,
                 unsigned int heat) {
  ac_heat = heat;
  unsigned int ac_msbits1 = 8;
  unsigned int ac_msbits2 = 8;
  unsigned int ac_msbits3 = 0;
  unsigned int ac_msbits4;
  if (ac_heat == 1)
    ac_msbits4 = 8;  // heating
  else
    ac_msbits4 = 0;  // cooling
  unsigned int ac_msbits5 =  (temperature < 15) ? 0 : temperature - 15;
  unsigned int ac_msbits6 = 0;

  if (air_flow <= 2) {
    if (kAc_Type == 0)
      ac_msbits6 = kAc_Flow_Tower[air_flow];
    else
      ac_msbits6 = kAc_Flow_Wall[air_flow];
  }

  // calculating using other values
  unsigned int ac_msbits7 = (ac_msbits3 + ac_msbits4 + ac_msbits5 +
                             ac_msbits6) & B00001111;
  ac_code_to_sent = ac_msbits1 << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits2) << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits3) << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits4) << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits5) << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits6) << 4;
  ac_code_to_sent = (ac_code_to_sent + ac_msbits7);

  Ac_Send_Code(ac_code_to_sent);

  ac_power_on = 1;
  ac_temperature = temperature;
  ac_flow = air_flow;
}

void Ac_Change_Air_Swing(int air_swing) {
  if (kAc_Type == 0) {
    if (air_swing == 1)
      ac_code_to_sent = 0x881316B;
    else
      ac_code_to_sent = 0x881317C;
  } else {
    if (air_swing == 1)
      ac_code_to_sent = 0x8813149;
    else
      ac_code_to_sent = 0x881315A;
  }
  Ac_Send_Code(ac_code_to_sent);
}

void Ac_Power_Down() {
  ac_code_to_sent = 0x88C0051;
  Ac_Send_Code(ac_code_to_sent);
  ac_power_on = 0;
}

void Ac_Air_Clean(int air_clean) {
  if (air_clean == '1')
    ac_code_to_sent = 0x88C000C;
  else
    ac_code_to_sent = 0x88C0084;

  Ac_Send_Code(ac_code_to_sent);

  ac_air_clean_state = air_clean;
}

void Power_On() {
  ac_code_to_sent = 0x880092B;
  Ac_Send_Code(ac_code_to_sent);
  ac_power_on = 1;
}

////////////////////////////////////////////////////////////////////////////
void remoteSignal() {
  if (prePower != power) {
    switch (power) {
      case 1:
        // power on
        Serial.println("Power ON");
        Power_On();
        prePower = 1;
        preFanSpd = 3;
        preSwing = 0;
        preTemp = 24;
        preAcmode = 2;
        preJetCool = 0;
        preESaving = 0;
        preClean = 0;

        break;
      case 0:
        // power off
        Serial.println("Power OFF");
        Ac_Power_Down();
        break;
      default:
        break;
    }
    prePower = power;
  }

  if (preTemp != temp) {
    switch (temp) {
      case 18:
        Serial.println("Tempetature set 18C");
        ac_temperature = 18;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 19:
        Serial.println("Tempetature set 19C");
        ac_temperature = 19;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 20:
        Serial.println("Tempetature set 20C");
        ac_temperature = 20;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 21:
        Serial.println("Tempetature set 21C");
        ac_temperature = 21;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 22:
        Serial.println("Tempetature set 22C");
        ac_temperature = 22;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 23:
        Serial.println("Tempetature set 23C");
        ac_temperature = 23;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 24:
        Serial.println("Tempetature set 24C");
        ac_temperature = 24;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 25:
        Serial.println("Tempetature set 25C");
        ac_temperature = 25;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 26:
        Serial.println("Tempetature set 26C");
        ac_temperature = 26;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 27:
        Serial.println("Tempetature set 27C");
        ac_temperature = 27;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 28:
        Serial.println("Tempetature set 28C");
        ac_temperature = 28;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 29:
        Serial.println("Tempetature set 29C");
        ac_temperature = 29;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 30:
        Serial.println("Tempetature set 30C");
        ac_temperature = 30;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      default:
        break;
    }
    preTemp = temp;
  }

  if (preSwing != swing) {
    switch (swing) {
      case 1:
        // swing on
        Serial.println("Swing On");
        Ac_Change_Air_Swing(1);
        break;
      case 0:
        // swing off
        Serial.println("Swing Off");
        Ac_Change_Air_Swing(0);
        break;
      default:
        break;
    }
    preSwing = swing;
  }

  if (preAcmode != acmode) {
    switch (acmode) {
      case 1:
        Serial.println("AC mode Dry");
        ac_code_to_sent = 0x8809924;
        Ac_Send_Code(ac_code_to_sent);
        break;
      case 2:
        Serial.println("AC mode Cool");
        ac_code_to_sent = 0x8808A24;
        Ac_Send_Code(ac_code_to_sent);
        break;
      case 3:
        Serial.println("AC mode Auto");
        ac_code_to_sent = 0x880B252;
        Ac_Send_Code(ac_code_to_sent);
        break;
      default:
        break;
    }
    preAcmode = acmode;
  }

  if (preFanSpd != fanSpd) {
    switch (fanSpd) {
      case 1:
        Serial.println("Fan Quiet");
        ac_flow = 0;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 2:
        Serial.println("Fan Medium");
        ac_flow = 1;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 3:
        Serial.println("Fan High");
        ac_flow = 2;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      case 4:
        Serial.println("Fan High");
        ac_flow = 3;
        Ac_Activate(ac_temperature, ac_flow, ac_heat);
        break;
      default:
        break;
    }
    preFanSpd = fanSpd;
  }
  if (preJetCool != jetCool) {
    switch (jetCool) {
      case 0:
        Serial.println("JET Cool");
        ac_code_to_sent = 0x880834F;
        Ac_Send_Code(ac_code_to_sent);
        break;
      case 1:
        Serial.println("JET Cool");
        ac_code_to_sent = 0x880834F;
        Ac_Send_Code(ac_code_to_sent);
        break;
      default:
        break;
    }
    preJetCool = jetCool;
  }

  if (preESaving != eSaving) {
    switch (eSaving) {
      case 0:
        Serial.println("Energy Saving");
        ac_code_to_sent = 0x8810045;
        Ac_Send_Code(ac_code_to_sent);
        break;
      case 1:
        Serial.println("Energy Saving");
        ac_code_to_sent = 0x8810056;
        Ac_Send_Code(ac_code_to_sent);
        break;
      default:
        break;
    }
    preESaving = eSaving;
  }

  if (preClean != clean) {
    switch (clean) {
      case 0:
        Serial.println("Energy Saving");
        Ac_Air_Clean(0);
        break;
      case 1:
        Serial.println("Energy Saving");
        Ac_Air_Clean(1);
        break;
      default:
        break;
    }
    preClean = clean;
  }
}

void getSettings() {
  power = Firebase.getInt("acSettings/power");
  temp = Firebase.getInt("acSettings/temp");
  fanSpd = Firebase.getInt("acSettings/fanSpd");
  swing = Firebase.getInt("acSettings/swing");
  acmode = Firebase.getInt("acSettings/mode");
  jetCool = Firebase.getInt("acSettings/jetCool");
  eSaving = Firebase.getInt("acSettings/eSaving");
  clean = Firebase.getInt("acSettings/clean");

  Serial.print("Power : ");
  Serial.print(power);
  Serial.print("\t");
  Serial.print("Temp : ");
  Serial.print(temp);
  Serial.print("\t");
  Serial.print("Fan Speed : ");
  Serial.print(fanSpd);
  Serial.print("\t");
  Serial.print("Swing : ");
  Serial.print(swing);
  Serial.print("\t");
  Serial.print("Mode : ");
  Serial.print(acmode);
  Serial.print("\t");
  Serial.print("Jet Cool : ");
  Serial.print(jetCool);
  Serial.print("\t");
  Serial.print("E.Saving : ");
  Serial.print(eSaving);
  Serial.print("\t");
  Serial.print("Clean : ");
  Serial.println(clean);

  if (Firebase.failed()) {
    Serial.print("Getting Value Failed : ");
    Serial.println(Firebase.error());
    firebasereconnect();
    return;
  }
  //delay(100);
}

void firebasereconnect() {
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  delay(10);
}

void wifiConnect() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    digitalWrite(On_Board_LED, LOW);
    delay(250);
    digitalWrite(On_Board_LED, HIGH);
    delay(250);
    Serial.print(".");
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

}

void wifiStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected...");
    wifiConnect();
  }
}

////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  pinMode(On_Board_LED, OUTPUT);
  Serial.println('\n');

  wifiConnect();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  delay(10);

  irsend.begin();
#if ESP8266
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
#else  // ESP8266
  Serial.begin(115200, SERIAL_8N1);
#endif  // ESP8266

  ////////////////////////////////////

  //Serial.begin(115200);
  //delay(1000);
  irsend.begin();


}

void loop() {
  wifiStatus();
  getSettings();
  remoteSignal();
}
