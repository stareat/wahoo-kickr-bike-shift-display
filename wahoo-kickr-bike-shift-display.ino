#include "NotoSansBold15.h"
#include "BLEDevice.h"
#include <SPI.h>
#include <TFT_eSPI.h>

//TFT
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprDebug = TFT_eSprite(&tft);
TFT_eSprite sprGearGraph = TFT_eSprite(&tft);
TFT_eSprite sprGearText = TFT_eSprite(&tft);
TFT_eSprite sprSlopeText = TFT_eSprite(&tft);

//FONTS
#define small NotoSansBold15

//COLORS
#define myColorGearBorder 0xFFFF
#define myColorGearSelected 0xEB61
#define myColorPurple 0xC415
#define myColorFont 0xFFFF
#define myColorBgFont 0x02AE
#define myColorBackground 0x5454

// BLE Server name
#define bleServerName "KICKR BIKE SHIFT 720C"

// BLE Service (UUID is case sensitive)
static BLEUUID serviceUUID1("a026ee0d-0a7d-4ab3-97fa-f1500f9feb8b");
// BLE Characteristics
static BLEUUID charUUID11("a026e03a-0a7d-4ab3-97fa-f1500f9feb8b");
//static BLEUUID charUUID12("a026e039-0a7d-4ab3-97fa-f1500f9feb8b");
//static BLEUUID charUUID13("a026e03c-0a7d-4ab3-97fa-f1500f9feb8b");

// BLE Service
//static BLEUUID serviceUUID2("a026ee0b-0a7d-4ab3-97fa-f1500f9feb8b");
// BLE Characteristics
//static BLEUUID charUUID21("a026e037-0a7d-4ab3-97fa-f1500f9feb8b");

// BLE Service
//static BLEUUID serviceUUID3("a026ee06-0a7d-4ab3-97fa-f1500f9feb8b");
// BLE Characteristics
//static BLEUUID charUUID31("a026e023-0a7d-4ab3-97fa-f1500f9feb8b");
//static BLEUUID charUUID32("a026e018-0a7d-4ab3-97fa-f1500f9feb8b");

// Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

// Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;

// Characteristicd that we want to read
BLERemoteCharacteristic* pRemoteCharacteristic_1;

void DisplayText(String myDebug)
{
  sprDebug.createSprite(320, 170);  
  sprDebug.fillSprite(TFT_BLACK);
  sprDebug.fillScreen(TFT_BLACK);

  sprDebug.loadFont(small);
  sprDebug.setCursor(10, 10);
  sprDebug.setTextSize(2);
  sprDebug.setTextColor(TFT_WHITE);
  //Output to TFT
  sprDebug.println(myDebug);
  //Output to Serian Monitor
  Serial.println(myDebug);

  sprDebug.pushSprite(0, 0);
  //sprDebug.deleteSprite();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) {
    DisplayText("We are connected");    
  }

  void onDisconnect(BLEClient* pClient) {
    connected = false;
    DisplayText("We are disconnected");
  }
};

// Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
  String myDisplay = "Forming a connection to " + String(pAddress.toString().c_str());
  DisplayText(myDisplay);  

  BLEClient* pClient = BLEDevice::createClient();
  DisplayText("We have created the Client");

  //Status 
  pClient->setClientCallbacks(new MyClientCallback());

  //pClient->connect(pAddress);
  pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM);
  DisplayText("We have connected to Server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID1);
  if (pRemoteService == nullptr) {
    myDisplay = "Failed to find our service UUID: " + String(serviceUUID1.toString().c_str());
    DisplayText(myDisplay);  
    pClient->disconnect();
    return false;
  }
  DisplayText("Found our Service");

  connected = true;
  pRemoteCharacteristic_1 = pRemoteService->getCharacteristic(charUUID11);
  if(connectCharacteristic(pRemoteService, pRemoteCharacteristic_1) == false) {
    connected = false;  
  }

  if(connected == false) {
    pClient-> disconnect();
    DisplayText("Characteristic UUID not found");
    return false;
  }  
  return true;
}

bool connectCharacteristic(BLERemoteService* pRemoteService, BLERemoteCharacteristic* pRemoteCharacteristic) {
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  if (pRemoteCharacteristic == nullptr) {
    DisplayText("Failed to find our characteristic UUID");
    return false;
  }
  DisplayText("Found our Characteristics");

  // Assign callback functions for the Characteristic(s)
  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback1);

  return true;
}

// Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String myDisplay = "BLE Advertised Device found: " + String(advertisedDevice.toString().c_str());
    DisplayText(myDisplay);      

    if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
      DisplayText("Found the Device and are connecting ...");
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect   
    }
  }
};

// When the BLE Server sends a new value reading with the notify property
int currentFrontGear = 0;
int currentRearGear = 0;
static void notifyCallback1(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // Gears
  // 2 ... # Selected Gear in Front
  // 3 ... # Selected Gear in Rear
  // 4 ... # Gears in Front
  // 5 ... # Gears in Rear

  //String myDisplay = "Gears:" + String(myFrontGear) + ":" + String(myRearGear);
  //DisplayText(myDisplay);

  if (currentFrontGear != pData[2] || currentRearGear != pData[3]) {
    GearGraph(pData[4], pData[5], pData[2], pData[3]);
    GearText(pData[2], pData[3]);  
  }
  currentFrontGear = pData[2];
  currentRearGear = pData[3];  

  /*
  for (int i = 0; i < length; i++) {
    Serial.print(i);
    Serial.print(":");
    Serial.printf("%d\n", pData[i]);
  }
  Serial.println("------");
  */
}

void setup() {
  Serial.begin(250000);

  //Screen Setup
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_MAGENTA);
  
  //Init BLE device
  BLEDevice::init("");
  DisplayText("Initializing Application");
 
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

// Graphics ---------------------------------------------
void GearGraph(int myFrontGears, int myRearGears, int selectedFrontGear, int selectedRearGear){ 
  //create sprite with size of screen
  sprGearGraph.createSprite(320, 170);
  //Background Color
  sprGearGraph.fillSprite(myColorBackground);
  
  #define myGearWidth 12
  #define myGearHeight 110
  #define myGearSpacing -1
  #define myGearX 140
  #define myGearY 30

  //FRONT GEARS (CHAINRINGS/CRANKSET)
  for (int i = 0; i < (myFrontGears); i++) {
    if(selectedFrontGear == i) {
      sprGearGraph.fillRect(myGearX+i*(myGearWidth+myGearSpacing), myGearY+(myFrontGears-i-1)*(myGearHeight/(myFrontGears*2)), myGearWidth, (i+1)*(myGearHeight/myFrontGears), myColorGearSelected);
      sprGearGraph.drawRect(myGearX+i*(myGearWidth+myGearSpacing), myGearY+(myFrontGears-i-1)*(myGearHeight/(myFrontGears*2)), myGearWidth, (i+1)*(myGearHeight/myFrontGears), myColorGearBorder);
    } else{
      sprGearGraph.drawRect(myGearX+i*(myGearWidth+myGearSpacing), myGearY+(myFrontGears-i-1)*(myGearHeight/(myFrontGears*2)), myGearWidth, (i+1)*(myGearHeight/myFrontGears), myColorGearBorder); 
    }
  }

  //REAR GEARS (CASSETTE)
  for (int i = 0; i < (myRearGears); i++) {
    if(selectedRearGear == i) {
      sprGearGraph.fillRect(20+(myGearX+(myFrontGears*myGearWidth)+(myFrontGears*myGearSpacing))+i*(myGearWidth+myGearSpacing), myGearY+i*(myGearHeight/(myRearGears*2)), myGearWidth, myGearHeight-i*(myGearHeight/myRearGears), myColorGearSelected);
      sprGearGraph.drawRect(20+(myGearX+(myFrontGears*myGearWidth)+(myFrontGears*myGearSpacing))+i*(myGearWidth+myGearSpacing), myGearY+i*(myGearHeight/(myRearGears*2)), myGearWidth, myGearHeight-i*(myGearHeight/myRearGears), myColorGearBorder);

    } else{  
      sprGearGraph.drawRect(20+(myGearX+(myFrontGears*myGearWidth)+(myFrontGears*myGearSpacing))+i*(myGearWidth+myGearSpacing), myGearY+i*(myGearHeight/(myRearGears*2)), myGearWidth, myGearHeight-i*(myGearHeight/myRearGears), myColorGearBorder);
    }   
  }
    
  sprGearGraph.pushSprite(0, 0);
  sprGearGraph.deleteSprite();
}

void GearText(int selectedFrontGear, int selectedRearGear)
{
  // Size of sprGearGraph
  #define IWIDTH  130
  #define IHEIGHT 170

  sprGearText.setColorDepth(16);
  sprGearText.createSprite(IWIDTH, IHEIGHT);
  sprGearText.fillSprite(myColorBgFont);
  // Draw a background
  sprGearText.fillRect(0, 0, IWIDTH, IHEIGHT, myColorBgFont);

  // Set text coordinate datum to middle centre
  sprGearText.setTextColor(myColorFont, myColorBgFont);
  sprGearText.setTextDatum(MC_DATUM);
  //sprGearText.drawString("SELECTED GEAR", IWIDTH/2, 15, 2);
  sprGearText.drawString(String(selectedFrontGear+1) + ":" + String(selectedRearGear+1), IWIDTH/2, IHEIGHT/2, 7);

  sprGearText.pushSprite(0, 0);
  sprGearText.deleteSprite();
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {    
    if (connectToServer(*pServerAddress)) {
      DisplayText("Connected to BLE Server");
      //String newValue = "Time since boot: " + String(millis()/1000);
      //pRemoteCharacteristic_1->writeValue(newValue.c_str(), newValue.length());      
      //connected = true;
    } else {
      DisplayText("Failed to connect - Restart to scan");
    }    
    doConnect = false;
  }
 
  delay(1000); // Delay a second between loops.
}