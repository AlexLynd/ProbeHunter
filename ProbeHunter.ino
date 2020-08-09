#include "./esppl_functions.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

Adafruit_SSD1306 display(-1);
#if (SSD1306_LCDHEIGHT != 64)
#error("Incorrect screen height, fix Adafruit_SSD1306.h"); // 128x64 OLED
#endif

#define ltBtn D7
#define mdBtn D6 
#define rtBtn D5 

int ltBtnState = 0; int pLtState = HIGH;
int mdBtnState = 0; int pMdState = HIGH;
int rtBtnState = 0; int pRtState = HIGH;

unsigned long prevTime;
unsigned long currTime;

uint8_t dev; uint8_t sel; uint8_t pos;
uint8_t tIndex = 0; // scroll index

String packet[7];
String devices[100][3]; int devCnt = 0;
String srcMac = "";
String ssid = "";
String sourceAddy;
String dest;
char srcOctet[2];
char destOctet[2];
int addr, fst, ft;
String pktType;

boolean probesOnly = false;

// current screen
boolean scan     = true;
boolean list     = false;
boolean toggle   = false;
boolean settings = false;

void cb(esppl_frame_info *info) {
    ssid = "";
    sourceAddy = "";  // source MAC
    
    for (int i= 0; i< 6; i++) { 
      sprintf(srcOctet, "%02x", info->sourceaddr[i]); 
      sourceAddy+=srcOctet;
    }

    dest = "";   // dest MAC
    for (int i= 0; i< 6; i++) { 
      sprintf(destOctet, "%02x", info->receiveraddr[i]); dest+=destOctet;
    }
//    Serial.println(sourceAddy);

    if (info->ssid_length > 0) {
     for (int i= 0; i< info->ssid_length; i++) { ssid+= (char) info->ssid[i]; }
    }
    packet[0] = (String) info->frametype;
    packet[1] = (String) info->framesubtype;
    packet[2] = sourceAddy;
    packet[3] = dest;
    packet[4] = info->rssi;
    packet[5] = info->channel;
    packet[6] = ssid;

    currTime = millis();
    addr = isFound(sourceAddy,devices);
    if (addr==-1 and devCnt!=100) {
      devices[devCnt][0] = sourceAddy;
      devices[devCnt][1] = info->rssi;
      devices[devCnt][2] = currTime;
      devCnt++;
    }
    else if (addr > -1) {
      devices[addr][1] = info->rssi;
      devices[addr][2] = currTime;
    }
    currTime = millis();
    if (currTime-prevTime >= 60000) { // dump device list every minute
      prevTime = currTime;
      memset(devices, 0, sizeof(devices)); devCnt = 0;
      sel = 0; dev = 0;
    }     

}

void printPacket() {
  ft = packet[0].toInt(); fst = packet[1].toInt(); 
  if      (ft == 0 and (fst == 0 or fst == 1)) pktType = "Association";
  else if (ft == 0 and (fst == 2 or fst == 3)) pktType = "Re-Assoc";
  else if (ft == 0 and (fst == 4 or fst == 5)) pktType = "Probe";
  else if (ft == 0 and fst == 8 ) pktType = "Association";
  else if (ft == 0 and fst == 10) pktType = "Disassoc";
  else if (ft == 0 and fst == 11) pktType = "Authentication";
  else if (ft == 0 and fst == 12) pktType = "DeAuth";
  else if (ft == 0) pktType = "Management";
  else if (ft == 1) pktType = "Control";
  else if (ft == 2) pktType = "Data";
  else pktType = "Extension";
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Packet: "); display.println(pktType);
  display.print("Src : "); display.println(packet[2]); srcMac = packet[2];
  display.print("Dest: "); display.println(packet[3]);
  display.print("RSSI: "); display.print(packet[4]); 
  display.print("  Ch: "); display.println(packet[5]);
  if (packet[6].length() > 20) { display.println(packet[6].substring(0, 18 ) + "..."); }
  else { display.println(packet[6]); }
}

void updateScan() {
  printPacket();
  display.drawLine(0,53,128,53,WHITE);
  display.drawLine(26,53,26,64,WHITE);
  display.drawLine(68,53,68,64,WHITE);
  display.setCursor(0,55);
  display.print("List Toggle Settings");
  display.display();
}

void updateList() {
  dev = 0; sel = 0;
  mdBtnState = digitalRead(mdBtn);
  esppl_sniffing_start();

  while(mdBtnState) {
    for (int i = 1; i < 15; i++ ) {
      esppl_set_channel(i);
      while (esppl_process_frames()) {
        //
      }
    }
    
    ltBtnState = digitalRead(ltBtn);
    mdBtnState = digitalRead(mdBtn);
    rtBtnState = digitalRead(rtBtn);
    
    display.clearDisplay();

    pos = 4; // position to print devices 
    for (int i=dev; i<dev+5; i++) {
      display.setCursor(12,pos);
      if ((int) devices[i][0].length() > 0) {
        display.println((String) devices[i][0]+" ("+ (String) devices[i][1]+")");
        pos+=8;
      }
    }
    
    display.drawTriangle(10, 59, 15, 55, 15, 63, WHITE);
    display.fillTriangle(10, 59, 15, 55, 15, 63, WHITE);

    display.drawTriangle(118, 59, 113, 55, 113, 63, WHITE);
    display.fillTriangle(118, 59, 113, 55, 113, 63, WHITE);
    
    display.drawLine(0,53,128,53,WHITE);
    display.setCursor(0,55);
    display.drawLine(30,53,30,64,WHITE);
    display.drawLine(98,53,98,64,WHITE);

    display.setCursor(48,55);
    display.println("select");

    display.drawTriangle(5, (sel*8)+7, 0, (sel*8)+4, 0, (sel*8)+10, WHITE);
    display.display();
    if ((int) devices[dev+sel+1][0].length() > 0) {
      if (!rtBtnState and dev<94) {
        if (sel<4) sel++;
        else { sel = 0; dev+=5;}
      }
      else if (!rtBtnState and dev>94) {
        if (sel<4) sel++;
      }
    }
    if (!ltBtnState and dev>4) {
      if (sel>0) sel--;
      else { sel = 4; dev-=5; }
    }
    else if (!ltBtnState and dev<4) {
      if (sel>0) sel--;
    }
    currTime = millis();

    for (int i=0; i<100; i++){
      if (currTime-devices[i][2].toInt() >= 30000) devices[i][1] = "-??";

    }
  }
  pMdState = mdBtnState;
  list = false; toggle = true; // toggle modes
  srcMac = devices[dev+sel][0];
  
  display.clearDisplay(); // buffer
  display.setCursor(0,25);
  display.println("Waiting on packets...");
    display.drawTriangle(10, 59, 15, 55, 15, 63, WHITE);
  display.fillTriangle(10, 59, 15, 55, 15, 63, WHITE);

  display.drawTriangle(118, 59, 113, 55, 113, 63, WHITE);
  display.fillTriangle(118, 59, 113, 55, 113, 63, WHITE);
  
  display.drawLine(0,53,128,53,WHITE);
  display.setCursor(0,55);
  display.drawLine(30,53,30,64,WHITE);
  display.drawLine(98,53,98,64,WHITE);

  display.setCursor(48,55);
  display.println("toggle"); 
  display.display();
  
}

/* TOGGLE */
void updateToggle(String currMac) {
  printPacket();

  display.drawTriangle(10, 59, 15, 55, 15, 63, WHITE);
  display.fillTriangle(10, 59, 15, 55, 15, 63, WHITE);

  display.drawTriangle(118, 59, 113, 55, 113, 63, WHITE);
  display.fillTriangle(118, 59, 113, 55, 113, 63, WHITE);
  
  display.drawLine(0,53,128,53,WHITE);
  display.setCursor(0,55);
  display.drawLine(30,53,30,64,WHITE);
  display.drawLine(98,53,98,64,WHITE);

  display.setCursor(48,55);
  display.println("toggle");  
  display.display();
}

/* SETTINGS */
void updateStgs() {
  rtBtnState = digitalRead(rtBtn);
  Serial.println("here");
  display.clearDisplay();
  display.setCursor(25,20);
  display.println("Only Probe Reqs.");
  display.drawLine(0,53,128,53,WHITE);
  display.drawLine(40,53,40,64,WHITE);
  display.drawLine(88,53,88,64,WHITE);
  display.setCursor(48,55);
  display.println("toggle  done"); 
  display.drawRect(10,20,7,7,WHITE);
  display.display();

  // toggle box until right pressed
  while (pRtState == rtBtnState) {rtBtnState = digitalRead(rtBtn);}
  pRtState = rtBtnState;
  
  while (rtBtnState) {
    mdBtnState = digitalRead(mdBtn);
    rtBtnState = digitalRead(rtBtn);
    if (!mdBtnState and probesOnly and mdBtnState!=pMdState)       { probesOnly = false; }
    else if (!mdBtnState and !probesOnly and mdBtnState!=pMdState) { probesOnly = true; }
    pMdState = mdBtnState;
    if (probesOnly) display.fillRect(10,20,7,7,WHITE);
    else display.fillRect(11,21,5,5,BLACK);
    display.display();
    if (!rtBtnState) {
      Serial.println("pressed!");
    }
  }
   display.clearDisplay();
  display.drawLine(0,53,128,53,WHITE);
  display.drawLine(26,53,26,64,WHITE);
  display.drawLine(68,53,68,64,WHITE);
  display.setCursor(0,55);
display.print("List Toggle Settings");
display.setCursor(0,25);
  if (probesOnly) {
    
    display.println("Waiting for probes...");
   
  }
  else display.println("Waiting on packets...");
   display.display();
  pRtState = rtBtnState;
  settings = false; scan = true; // toggle to scan
  
}

int isFound(String bssid, String arr[][3]) {
  for (int i=0; i<100; i++){
    if (arr[i][0]==bssid) return i;
  }
  return -1;
}

void setup() {
  pinMode(ltBtn, INPUT);
  pinMode(mdBtn, INPUT);
  pinMode(rtBtn, INPUT);
  
  delay(500);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // OLED address
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.display();
  Serial.begin(115200);
  esppl_init(cb);
}

void loop() {
  esppl_sniffing_start();
         
  while (true) {
    for (int i = 1; i < 15; i++ ) {
      esppl_set_channel(i);
      while (esppl_process_frames()) {
        //
      }
    }

    ltBtnState = digitalRead(ltBtn);
    mdBtnState = digitalRead(mdBtn);
    rtBtnState = digitalRead(rtBtn);
    if (!ltBtnState and scan) {
      scan = false; list = true;
    }
    else if (!rtBtnState and scan and (pRtState!=rtBtnState)) {
      scan = false; settings = true;
    }
    else if (!mdBtnState and scan and (pMdState!=mdBtnState)) {
      scan = false; toggle = true;
      display.clearDisplay();
      display.drawTriangle(10, 59, 15, 55, 15, 63, WHITE);
      display.fillTriangle(10, 59, 15, 55, 15, 63, WHITE);
    
      display.drawTriangle(118, 59, 113, 55, 113, 63, WHITE);
      display.fillTriangle(118, 59, 113, 55, 113, 63, WHITE);
      
      display.drawLine(0,53,128,53,WHITE);
      display.setCursor(0,55);
      display.drawLine(30,53,30,64,WHITE);
      display.drawLine(98,53,98,64,WHITE);
    
      display.setCursor(48,55);
      display.println("toggle");  
      display.setCursor(0,17);
      display.println("Waiting on packets");
      display.setCursor(0,25);
      display.println("from " + packet[2]);
      display.display();
    }
    else if (!mdBtnState and toggle and (pMdState!=mdBtnState)) {
      toggle = false; scan = true;
    }
    pLtState = ltBtnState;
    pMdState = mdBtnState;
    pRtState = rtBtnState;
    if (scan)     { 
      if (probesOnly and (packet[0].toInt() ==0 and (packet[1].toInt() == 4 or packet[1].toInt() == 5))) { 
          updateScan(); 
      }
      else if (!probesOnly) {
        updateScan();
      }
      else {
        Serial.println("not printing");
      }
    }
    else if (list) { updateList(); }
    else if (settings) { updateStgs(); }
    else if (toggle)   { 
      if (packet[2]==srcMac) {
        updateToggle(srcMac); 
      }
      
    }
  }  
}
