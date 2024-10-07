#include "NotoSansBold15.h"
#include "PathwayGothicOneRegular65.h"
#include "PathwayGothicOneRegular55.h"
#include "BLEDevice.h"
#include <SPI.h>
#include <TFT_eSPI.h>

// TFT
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprDebug = TFT_eSprite(&tft);
TFT_eSprite sprGearingGraph = TFT_eSprite(&tft);
TFT_eSprite sprGearText = TFT_eSprite(&tft);
TFT_eSprite sprPowerKgText = TFT_eSprite(&tft);
TFT_eSprite sprCadenceText = TFT_eSprite(&tft);
TFT_eSprite sprPowerText = TFT_eSprite(&tft);

// RESOLUTION
// 320 x 170 ... LILYGO T-Display-S3 ESP32-S3
// 240 x 135 ... LILYGO TTGO T-Display ESP32
#define RESOLUTION_X 320
#define RESOLUTION_Y 170

// BUTTON BOOT
#define BUTTON_PIN_BOOT 0
int intLastButtonState;

/*
// BUTTON IO
// TTGO ... PIN 35
// S3   ... PIN 14
#define BUTTON_PIN_IO 14
bool bolButtonIO = false;
*/

//variables to keep track of the timing of recent interrupts
unsigned long buttonTime = 0;
unsigned long buttonTimeOld = 0; 

int intPower = 0;
// 0 = instant 1 = 3s, 2 = 10s
int intTogglePowerMode = 0;
unsigned long millisPower = 0;
unsigned long millisPowerOld = 0;  

// Weight Measurement
float flUserWeight = 1;

// Moving Average
#define BUF_SIZE 50
float array_calculate_avg(int * buf, int len);
int buf[BUF_SIZE] = {0};
int buf_index = 0;
int buf_size_toggle = 1;
float bufferMovingAverage = 0.0;
unsigned long millisAverage = 0;  
unsigned long millisAverageOld = 0;  

// FONTS
#define small NotoSansBold15
#define digits65 PathwayGothicOneRegular65
#define digits55 PathwayGothicOneRegular55

// COLORS in RGB565
// https://barth-dev.de/online/rgb565-color-picker/
// https://trolsoft.ru/en/articles/rgb565-color-picker
#define myColorGearBorder 0xFFFF
#define myColorGearSelected 0xEB61
#define myColorFont 0xFFFF
#define myColorBgGear 0x02AE
#define myColorBgGearGraph 0x5454
#define myColorBgPowerKg 0xBBD4
#define myColorBgCadence 0x75D1
#define myColorBgPower 0xC32D

// DEBUG
int AdvertisingIntervalNew = 0;
int AdvertisingIntervalOld = 0;
int AdvertisingInterval = 0;

// BLE Server name
#define bleServerName "KICKR BIKE SHIFT 5AF9"

// Cadence
// Crank Event Time
float NewCrankEventTime;
float DiffCrankEventTime;
float OldCrankEventTime;
float CrankEventTime;
// Crank Revolutions
float NewCrankRevolutions;
float OldCrankRevolutions;
float DiffCrankRevolutions;

//Our Cadence
int Cadence = 0;
int OldCadence = 0;
int prevCrankStaleness = 0;

// Gearing
// BLE Service (UUID is case sensitive)
static BLEUUID serviceUUID1("a026ee0d-0a7d-4ab3-97fa-f1500f9feb8b");
// BLE Characteristics
static BLEUUID charUUID11("a026e03a-0a7d-4ab3-97fa-f1500f9feb8b");

// Cycling Power Measurement
static BLEUUID serviceUUID2("00001818-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID21("00002a63-0000-1000-8000-00805f9b34fb");

// Weight Measurement
static BLEUUID serviceUUID3("0000181c-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID31("00002a98-0000-1000-8000-00805f9b34fb");

// Heart Rate Measurement
static BLEUUID serviceUUID4("0000180d-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID41("00002a37-0000-1000-8000-00805f9b34fb");

// Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

// Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;

// Characteristicd that we want to read
// Gearing
BLERemoteCharacteristic *pRemoteCharacteristic_1;
// Cycling Power Measurement
BLERemoteCharacteristic *pRemoteCharacteristic_2;
// Weight Measurement
BLERemoteCharacteristic *pRemoteCharacteristic_3;

void DisplayText(String myDebug) {
  sprDebug.createSprite(RESOLUTION_X, RESOLUTION_Y);
  sprDebug.fillSprite(TFT_BLACK);
  // sprDebug.fillScreen(TFT_BLACK);

  sprDebug.loadFont(small);
  sprDebug.setCursor(10, 10);
  sprDebug.setTextSize(2);
  sprDebug.setTextColor(TFT_WHITE);
  // Output to TFT
  sprDebug.println(myDebug);
  // Output to Serian Monitor
  Serial.println(myDebug);

  sprDebug.pushSprite(0, 0);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pClient) {
    DisplayText("We are connected");

    // Change the parameters if we don't need fast response times
    //pClient->updateConnParams(...,...,...,...);
  }

  void onDisconnect(BLEClient *pClient) {
    connected = false;
    DisplayText("We are disconnected: " + String(pClient->getPeerAddress().toString().c_str()));
  }
};

// Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
  String myDisplay = "Forming a connection to " + String(pAddress.toString().c_str());
  DisplayText(myDisplay);

  BLEClient *pClient = BLEDevice::createClient();
  DisplayText("We have created the Client");

  // Status
  pClient->setClientCallbacks(new MyClientCallback());
  
  // pClient->setConnectionParams(...,...,...,...);
  // Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
  // pClient->setConnectTimeout(5);  

  pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM);

  if(!pClient->isConnected()) {
    if (!pClient->connect(pAddress)) {
      DisplayText("Failed to connect");
      return false;
    }
  }  

  DisplayText("We have connected to Server: " + String(pClient->getPeerAddress().toString().c_str()) + " (RSSI: " + String(pClient->getRssi()) + ")");  

  // Gearing
  BLERemoteService *pRemoteService1 = pClient->getService(serviceUUID1);
  if (pRemoteService1 == nullptr) {
    myDisplay = "Failed to find our service UUID1: " + String(serviceUUID1.toString().c_str());
    DisplayText(myDisplay);
    pClient->disconnect();
    return false;
  }
  DisplayText("Found our Service UUID1");

  // Cycling Power Measurement
  BLERemoteService *pRemoteService2 = pClient->getService(serviceUUID2);
  if (pRemoteService2 == nullptr) {
    myDisplay = "Failed to find our service UUID2: " + String(serviceUUID2.toString().c_str());
    DisplayText(myDisplay);
    pClient->disconnect();
    return false;
  }
  DisplayText("Found our Service UUID2");

  // Weight Measurement
  BLERemoteService *pRemoteService3 = pClient->getService(serviceUUID3);
  if (pRemoteService3 == nullptr) {
    myDisplay = "Failed to find our service UUID3: " + String(serviceUUID3.toString().c_str());
    DisplayText(myDisplay);
    pClient->disconnect();
    return false;
  }
  DisplayText("Found our Service UUID3");

  connected = true;

  // Gearing
  pRemoteCharacteristic_1 = pRemoteService1->getCharacteristic(charUUID11);
  if (connectCharacteristic1(pRemoteService1, pRemoteCharacteristic_1) == false) {
    connected = false;
  }

  // Cycling Power Measurement
  pRemoteCharacteristic_2 = pRemoteService2->getCharacteristic(charUUID21);
  if (connectCharacteristic2(pRemoteService2, pRemoteCharacteristic_2) == false) {
    connected = false;
  }

  // Weight Measurement
  pRemoteCharacteristic_3 = pRemoteService3->getCharacteristic(charUUID31);
  if (connectCharacteristic3(pRemoteService3, pRemoteCharacteristic_3) == false) {
    connected = false;
  }

  if (connected == false) {
    pClient->disconnect();
    DisplayText("Characteristic UUID(s) not found");
    return false;
  }

  return true;
}

// Gearing
bool connectCharacteristic1(BLERemoteService *pRemoteService, BLERemoteCharacteristic *pRemoteCharacteristic) {
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  if (pRemoteCharacteristic == nullptr) {
    DisplayText("Failed to find our characteristic UUID1");
    return false;
  }
  DisplayText("Found our Characteristic 1");

  // Assign callback functions for the Characteristic(s)
  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyGearing);

  return true;
}

// Cycling Power Measurement
bool connectCharacteristic2(BLERemoteService *pRemoteService, BLERemoteCharacteristic *pRemoteCharacteristic) {
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  if (pRemoteCharacteristic == nullptr) {
    DisplayText("Failed to find our characteristic UUID2");
    return false;
  }
  DisplayText("Found our Characteristic 2");

  // Assign callback functions for the Characteristic(s)
  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCyclingPowerMeasurement);

  return true;
}

// Weight Measurement
bool connectCharacteristic3(BLERemoteService *pRemoteService, BLERemoteCharacteristic *pRemoteCharacteristic) {
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  if (pRemoteCharacteristic == nullptr) {
    DisplayText("Failed to find our characteristic UUID3");
    return false;
  }
  DisplayText("Found our Characteristic 3");

  // No callback needed - we only need to read the weight once
  if(pRemoteCharacteristic->canRead()) {

    // we're expecting a 2 Byte HEX value eg "D039" 
    std::string rxValue = pRemoteCharacteristic->readValue().c_str();

    /*
    // DEBUG
    Serial.println(rxValue[0],HEX);
    Serial.println(rxValue[1],HEX);
    */

    // Endianness is little-endian
    // so lets swap bytes and then combine two uint8_t as uint16_tbit
    // bitshift the left by 8 bits to make them the 8 most significant bits of our new value
    uint16_t tmp16 = (rxValue[1] << 8 | rxValue[0]);
    int intWeight = (int)(tmp16);
    flUserWeight = (float(intWeight) * 0.005);
    DisplayText("Cyclist weight: " + String(flUserWeight));
  }  

  return true;
}

// Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String myDisplay = "BLE Advertised Device found: " + String(advertisedDevice.toString().c_str());
    DisplayText(myDisplay);

    if (advertisedDevice.getName() == bleServerName) { 
      // Check if the name of the advertiser matches
      DisplayText("Found the Device and are connecting ...");
      // stop scan before connecting, we found what we are looking for
      advertisedDevice.getScan()->stop();      
      // Save the device reference (address of advertiser) in a global for the client to use
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      // Set indicator, stating that we are ready to connect
      doConnect = true;                                              
    }

    // Connect to 2nd Server
    // We have found a device, let us now see if it contains the service we are looking for.
    /*
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID0)) {
      //BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);  // not used?
      doConnect0 = true;
      doScan0 = true;
      Serial.print("Found serviceUUID0");
    } // Found our server    
    */
  }
};

// Gearing
// When the BLE Server sends a new value reading with the notify property
int currentFrontGear = 0;
int currentRearGear = 0;
static void notifyGearing(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  // Gears
  // 2 ... # Selected Gear in Front
  // 3 ... # Selected Gear in Rear
  // 4 ... # Gears in Front
  // 5 ... # Gears in Rear

  // String strDebug = "Gearing:" + String(pData[4]) + ":" + String(pData[5]) + "\n" + "Selected Gears:" + String(pData[2]) + ":" + String(pData[3]);
  // DisplayText(strDebug);

  if (currentFrontGear != pData[2] || currentRearGear != pData[3]) {
    // Draw Graphics
    GearingGraph(pData[4], pData[5], pData[2], pData[3]);
    GearingText(pData[2], pData[3]);
  }
  currentFrontGear = pData[2];
  currentRearGear = pData[3];

  /*
  // DEBUG
  for (int i = 0; i < length; i++) {
    Serial.print(i);
    Serial.print(":");
    Serial.printf("%d\n", pData[i]);
  }
  Serial.println("------");
  */
}

// Moving Average with a circular buffer
float array_calculate_avg(int * buf, int len) {
    int sum = 0;
    for (int i = 0; i < len; ++i) {
      sum += buf[i];        
    }
    return ((float) sum) / ((float) len);
}

// Cycling Power Measurement
static void notifyCyclingPowerMeasurement(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  // According to the GATT Specification Supplement (Bluetooth Specification) the following values are submitted
  // [0][1] ... Flags field = [3C][00] == "00000000 00111100" (Mandatory)
  // [2][3] ... Instantaneous Power (Mandatory)
  // [4][5] ... Accumulated Torque
  // [6][7][8][9] ... Wheel Revolution Data - Cumulative Wheel Revolutions
  // [10][11] ... Last Wheel Event Time
  // [12][13] ... Crank Revolution Data - Cumulative Crank Revolutions
  // [14][15] ... Crank Revolution Data - Last Crank Event Time

  // Instantaneous Power
  // Endianness is little-endian
  // so lets swap bytes and then combine two uint8_t as uint16_tbit
  // bitshift the left by 8 bits to make them the 8 most significant bits of our new value
  uint16_t tmp16 = (pData[3] << 8 | pData[2]);
  intPower = (int)(tmp16);

  // // DEBUG --------
  // AdvertisingIntervalNew = millis();
  // AdvertisingInterval = (AdvertisingIntervalNew-AdvertisingIntervalOld);
  // Serial.println("(" + String(AdvertisingIntervalNew) + " - " + String(AdvertisingIntervalOld) + ") = " + String(AdvertisingInterval) + " value --> " + String(intPower));
  // AdvertisingIntervalOld = AdvertisingIntervalNew;
  // END DEBUG ----  

  // Cadence
  // Last Crank Event Time (Unit is 1/1024 second)
  // Endianness is little-endian
  // so lets swap bytes and then combine two uint8_t as uint16_tbit
  // bitshift the left by 8 bits to make them the 8 most significant bits of our new value
  NewCrankEventTime = pData[14] + pData[15]*256;

  // Difference between new & old in 1/1024 seconds
  DiffCrankEventTime = NewCrankEventTime - OldCrankEventTime;

  // Preventing an overflow: uint16 can not exceed 65535 (maximum)
  if (DiffCrankEventTime < 0) { 
    DiffCrankEventTime = DiffCrankEventTime + 65535;
  }  
  
  // in seconds
  CrankEventTime = float(DiffCrankEventTime) / 1024; 

  // Cumulative Crank Revolutions
  NewCrankRevolutions = pData[12] + pData[13]*256;   
  //Difference between new % old 
  DiffCrankRevolutions = NewCrankRevolutions - OldCrankRevolutions;

  // Preventing an overflow: uint16 can not exceed 65535 (maximum)
  if (DiffCrankRevolutions < 0) { 
    DiffCrankRevolutions = DiffCrankRevolutions + 65535;
  }    

  // In Case Cadence Drops (notify between new crank revolution), we use OldCadence 
  if (CrankEventTime > 0) {
    prevCrankStaleness = 0;
    Cadence = DiffCrankRevolutions / CrankEventTime * 60;
    OldCadence = Cadence;
  } else if (CrankEventTime == 0 && prevCrankStaleness < 10 ) {
    Cadence = OldCadence;
    prevCrankStaleness += 1;
  } else if (prevCrankStaleness >= 10) {
    Cadence = 0;
  }

  //Just some init correction
  if (Cadence > 1000) { Cadence=0; }  
  
  // Serial.println("DiffCrankEventTime: " + String(DiffCrankEventTime));
  // Serial.println("CrankEventTime: " + String(CrankEventTime));
  // Serial.println("DiffCrankRevolutions: " + String(DiffCrankRevolutions));
  // Serial.println("Cadence Calc/Flat: " + String(DiffCrankRevolutions / CrankEventTime * 60) + " / " + String(Cadence) + " Rpm");
  // Serial.println("---");
  
  OldCrankEventTime = NewCrankEventTime;
  OldCrankRevolutions = NewCrankRevolutions;
}

/*
// IO Button Action
void IRAM_ATTR toggleButtonIO() {
  // Debounce Time (Debouncing an Interrupt)
  buttonTime = millis();  
  if (buttonTime - buttonTimeOld > 250) {
    buttonTimeOld = buttonTime;
    // NO ACTION HERE -> put it in loop()
    // NEVER put Serial etc. here!
    bolButtonIO = true;
  }
}
*/

void setup() {
  // Bit per second (baud) for serial data transmission
  Serial.begin(250000);

  // Screen Setup
  // tft.init();
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_MAGENTA);

/*
  // Buttons
  // https://lastminuteengineers.com/handling-esp32-gpio-interrupts-tutorial/
  // IO Button
  pinMode(BUTTON_PIN_IO, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN_IO), toggleButtonIO, FALLING);
*/

  // Init BLE device
  BLEDevice::init("");
  DisplayText("Initializing Application");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  // Amount of time (duration) between consecutive scans
  // pBLEScan->setInterval(1349);
  // Amount of time (duration) of the scan.
  // pBLEScan->setWindow(449);  
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

// Graphics ---------------------------------------------
void GearingGraph(int myFrontGears, int myRearGears, int selectedFrontGear, int selectedRearGear)
{
  // Size of Graph
  #define IWIDTH ((RESOLUTION_X/5)*3)
  #define IHEIGHT ((RESOLUTION_Y / 5)*3)

  sprGearingGraph.setColorDepth(16);
  // create sprite with size of screen
  sprGearingGraph.createSprite(IWIDTH, IHEIGHT);
  // Background Color
  sprGearingGraph.fillSprite(myColorBgGearGraph);

  int myGearWidth = (IWIDTH - 30) / (myFrontGears + myRearGears);
  #define myGearHeight (RESOLUTION_Y - 90)
  #define myGearSpacing -1
  #define myGearX 15
  #define myGearY 10

  // FRONT GEARS (CHAINRINGS/CRANKSET)
  for (int i = 0; i < (myFrontGears); i++) {
    if (selectedFrontGear == i) {
      sprGearingGraph.fillRect(myGearX + i * (myGearWidth + myGearSpacing), myGearY + (myFrontGears - i - 1) * (myGearHeight / (myFrontGears * 2)), myGearWidth, (i + 1) * (myGearHeight / myFrontGears), myColorGearSelected);
      sprGearingGraph.drawRect(myGearX + i * (myGearWidth + myGearSpacing), myGearY + (myFrontGears - i - 1) * (myGearHeight / (myFrontGears * 2)), myGearWidth, (i + 1) * (myGearHeight / myFrontGears), myColorGearBorder);
    } else {
      sprGearingGraph.drawRect(myGearX + i * (myGearWidth + myGearSpacing), myGearY + (myFrontGears - i - 1) * (myGearHeight / (myFrontGears * 2)), myGearWidth, (i + 1) * (myGearHeight / myFrontGears), myColorGearBorder);
    }
  }

  // REAR GEARS (CASSETTE)
  for (int i = 0; i < (myRearGears); i++) {
    if (selectedRearGear == i) {
      sprGearingGraph.fillRect(20 + (myGearX + (myFrontGears * myGearWidth) + (myFrontGears * myGearSpacing)) + i * (myGearWidth + myGearSpacing), myGearY + i * (myGearHeight / (myRearGears * 2)), myGearWidth, myGearHeight - i * (myGearHeight / myRearGears), myColorGearSelected);
      sprGearingGraph.drawRect(20 + (myGearX + (myFrontGears * myGearWidth) + (myFrontGears * myGearSpacing)) + i * (myGearWidth + myGearSpacing), myGearY + i * (myGearHeight / (myRearGears * 2)), myGearWidth, myGearHeight - i * (myGearHeight / myRearGears), myColorGearBorder);
    } else {
      sprGearingGraph.drawRect(20 + (myGearX + (myFrontGears * myGearWidth) + (myFrontGears * myGearSpacing)) + i * (myGearWidth + myGearSpacing), myGearY + i * (myGearHeight / (myRearGears * 2)), myGearWidth, myGearHeight - i * (myGearHeight / myRearGears), myColorGearBorder);
    }
  }

  sprGearingGraph.pushSprite(((RESOLUTION_X/5)*2), 0);
  sprGearingGraph.deleteSprite();
}

void GearingText(int selectedFrontGear, int selectedRearGear) {
  // Size of Graph
  #define IWIDTH (((RESOLUTION_X/5)*3)/2)
  #define IHEIGHT ((RESOLUTION_Y / 5)*2)

  sprGearText.setColorDepth(16);
  sprGearText.createSprite(IWIDTH, IHEIGHT);
  sprGearText.fillSprite(myColorBgGear);
  // Draw a background
  sprGearText.fillRect(0, 0, IWIDTH, IHEIGHT, myColorBgGear);

  sprGearText.loadFont(digits55);
  sprGearText.setTextColor(myColorFont, myColorBgGear);
  sprGearText.setTextDatum(MC_DATUM);
  sprGearText.drawString(String(selectedFrontGear + 1) + ":" + String(selectedRearGear + 1), IWIDTH / 2, (IHEIGHT / 2) + 5);
  sprGearText.unloadFont();

  sprGearText.pushSprite(((RESOLUTION_X/5)*2)+IWIDTH, ((RESOLUTION_Y / 5)*3));
  sprGearText.deleteSprite();
}

void PowerText(String strPower) {
  // Size of Graph
  #define IWIDTH (((RESOLUTION_X/5)*3)/2)
  #define IHEIGHT ((RESOLUTION_Y / 5)*2)

  sprPowerText.setColorDepth(16);
  sprPowerText.createSprite(IWIDTH, IHEIGHT);
  sprPowerText.fillSprite(myColorBgPower);
  // Draw a background
  sprPowerText.fillRect(0, 0, IWIDTH, IHEIGHT, myColorBgPower);

  sprPowerText.loadFont(digits55);
  sprPowerText.setTextColor(myColorFont, myColorBgPower);
  sprPowerText.setTextDatum(MC_DATUM);
  sprPowerText.drawString(strPower, IWIDTH / 2, (IHEIGHT / 2) + 5);
  sprPowerText.unloadFont();

  sprPowerText.pushSprite(((RESOLUTION_X/5)*2), ((RESOLUTION_Y / 5)*3));
  sprPowerText.deleteSprite();  
}

void PowerKgText(String strPower) {
  // Size of Graph
  #define IWIDTH ((RESOLUTION_X/5)*2)
  #define IHEIGHT (RESOLUTION_Y / 2)

  sprPowerKgText.setColorDepth(16);
  sprPowerKgText.createSprite(IWIDTH, IHEIGHT);
  sprPowerKgText.fillSprite(myColorBgPowerKg);
  // Draw a background
  sprPowerKgText.fillRect(0, 0, IWIDTH, IHEIGHT, myColorBgPowerKg);

  sprPowerKgText.loadFont(digits65);
  sprPowerKgText.setTextColor(myColorFont, myColorBgPowerKg);
  sprPowerKgText.setTextDatum(MC_DATUM);
  sprPowerKgText.drawString(strPower, IWIDTH / 2, (IHEIGHT / 2) + 5);
  sprPowerKgText.unloadFont();

  sprPowerKgText.pushSprite(0, IHEIGHT);
  sprPowerKgText.deleteSprite();
}

void CadenceText(String strCadence)
{
  // Size of Graph
  #define IWIDTH ((RESOLUTION_X/5)*2)
  #define IHEIGHT (RESOLUTION_Y / 2)

  sprCadenceText.setColorDepth(16);
  sprCadenceText.createSprite(IWIDTH, IHEIGHT);
  sprCadenceText.fillSprite(myColorBgCadence);
  // Draw a background
  sprCadenceText.fillRect(0, 0, IWIDTH, IHEIGHT, myColorBgCadence);

  sprCadenceText.loadFont(digits65);
  sprCadenceText.setTextColor(myColorFont, myColorBgCadence);
  sprCadenceText.setTextDatum(MC_DATUM);
  sprCadenceText.drawString(strCadence, IWIDTH / 2, (IHEIGHT / 2) + 5);
  sprCadenceText.unloadFont();

  sprCadenceText.pushSprite(0, 0);
  sprCadenceText.deleteSprite();
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      DisplayText("Connected to BLE Server");
    } else {
      DisplayText("Failed to connect - Restart to scan");
    }
    doConnect = false;
  }

/*
  // BUTTON ACTION
  if (bolButtonIO) {
    // ACTION

    bolButtonIO = false;
  }  
*/

  int intBootButtonState = digitalRead(BUTTON_PIN_BOOT);
  if (intLastButtonState != intBootButtonState) {
    // Debounce Time
    delay(50);
    if (intBootButtonState == LOW) {
      // Toggle Button State
      intTogglePowerMode++;
      if (intTogglePowerMode > 2) intTogglePowerMode = 0; 

      // Press Action
      switch (intTogglePowerMode) {
        case 0:
          // instant
          PowerText("INST");
          PowerKgText("INST");
          buf_size_toggle = 1;
          break;
        case 1:      
          // 3s
          PowerText("3s");
          PowerKgText("3s");        
          buf_size_toggle = 15;
          break;
        case 2:
          // 10s
          PowerText("10s");
          PowerKgText("10s");                
          buf_size_toggle = BUF_SIZE;
          break;
      }        
    } else {
      // Release Action
    }
    intLastButtonState = intBootButtonState;
  }  

  // Moving Average
  // Notifies are sent between 100-300ms - we update every 200ms so we don't miss a update
  millisAverage = millis();
  if (millisAverage - millisAverageOld >= 200) {
    // Reset the index and start over.
    if (buf_index >= buf_size_toggle) buf_index = 0;

    buf[buf_index++] = intPower;
    bufferMovingAverage = array_calculate_avg(buf, buf_size_toggle);
    //Serial.println("Index " + String(buf_index) + " Timespan " + String(millisAverage - millisAverageOld) + " Buffer Size: " + String(buf_size_toggle) + " Average: " + String(bufferMovingAverage));

    millisAverageOld = millisAverage;
  }

  // UPDATE DISPLAY everey x ms
  millisPower = millis();
  if (millisPower-millisPowerOld >= 1000){ 
    // POWER
    if (intTogglePowerMode == 0) {
      PowerText(String(intPower));
      PowerKgText(String((float(intPower) / float(flUserWeight)), 1));
    } else {
      PowerText(String(bufferMovingAverage, 0));
      PowerKgText(String((float(bufferMovingAverage) / float(flUserWeight)), 1));
    }

    // CADENCE
    CadenceText(String(Cadence));

    millisPowerOld = millisPower;
  }    
}
