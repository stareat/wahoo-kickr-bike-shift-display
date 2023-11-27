#include "NotoSansBold15.h"
#include "PathwayGothicOneRegular65.h"
#include "BLEDevice.h"
#include <SPI.h>
#include <TFT_eSPI.h>

// TFT
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprDebug = TFT_eSprite(&tft);
TFT_eSprite sprGearGraph = TFT_eSprite(&tft);
TFT_eSprite sprGearText = TFT_eSprite(&tft);
TFT_eSprite sprPowerText = TFT_eSprite(&tft);

// RESOLUTION
// 170 x 320 ... LILYGO T-Display-S3 ESP32-S3
// 135 x 240 ... LILYGO TTGO T-Display ESP32
#define RESOLUTION_X 320
#define RESOLUTION_Y 170

// BUTTON
#define BUTTON_PIN 0
int intLastButtonState;
bool bolToggleScreen = false;

// FONTS
#define small NotoSansBold15
#define digits PathwayGothicOneRegular65

// COLORS
// https://barth-dev.de/online/rgb565-color-picker/
#define myColorGearBorder 0xFFFF
#define myColorGearSelected 0xEB61
#define myColorPurple 0xC415
#define myColorFont 0xFFFF
#define myColorBgFont 0x02AE
#define myColorBgPower 0xBBD4
#define myColorBackground 0x5454

// BLE Server name
#define bleServerName "KICKR BIKE SHIFT 720C"
// Rider Weight
#define WEIGHT 75

// Gearing
// BLE Service (UUID is case sensitive)
static BLEUUID serviceUUID1("a026ee0d-0a7d-4ab3-97fa-f1500f9feb8b");
// BLE Characteristics
static BLEUUID charUUID11("a026e03a-0a7d-4ab3-97fa-f1500f9feb8b");

// Power Measurement
static BLEUUID serviceUUID2("00001818-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID21("00002a63-0000-1000-8000-00805f9b34fb");

// Weigt Measurement
static BLEUUID serviceUUID3("0000181c-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID31("00002a98-0000-1000-8000-00805f9b34fb");

// Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

// Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;

// Characteristicd that we want to read
// Gearing
BLERemoteCharacteristic *pRemoteCharacteristic_1;
// Power
BLERemoteCharacteristic *pRemoteCharacteristic_2;

void DisplayText(String myDebug)
{
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
  // sprDebug.deleteSprite();
}

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pClient)
  {
    DisplayText("We are connected");
  }

  void onDisconnect(BLEClient *pClient)
  {
    connected = false;
    DisplayText("We are disconnected");
  }
};

// Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress)
{
  String myDisplay = "Forming a connection to " + String(pAddress.toString().c_str());
  DisplayText(myDisplay);

  BLEClient *pClient = BLEDevice::createClient();
  DisplayText("We have created the Client");

  // Status
  pClient->setClientCallbacks(new MyClientCallback());

  // pClient->connect(pAddress);
  pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM);
  DisplayText("We have connected to Server");

  // Gearing
  BLERemoteService *pRemoteService1 = pClient->getService(serviceUUID1);
  if (pRemoteService1 == nullptr)
  {
    myDisplay = "Failed to find our service UUID1: " + String(serviceUUID1.toString().c_str());
    DisplayText(myDisplay);
    pClient->disconnect();
    return false;
  }
  DisplayText("Found our Service UUID1");

  // Power
  BLERemoteService *pRemoteService2 = pClient->getService(serviceUUID2);
  if (pRemoteService2 == nullptr)
  {
    myDisplay = "Failed to find our service UUID2: " + String(serviceUUID2.toString().c_str());
    DisplayText(myDisplay);
    pClient->disconnect();
    return false;
  }
  DisplayText("Found our Service UUID2");

  connected = true;

  // Gearing
  pRemoteCharacteristic_1 = pRemoteService1->getCharacteristic(charUUID11);
  if (connectCharacteristic1(pRemoteService1, pRemoteCharacteristic_1) == false)
  {
    connected = false;
  }

  // Power
  pRemoteCharacteristic_2 = pRemoteService2->getCharacteristic(charUUID21);
  if (connectCharacteristic2(pRemoteService2, pRemoteCharacteristic_2) == false)
  {
    connected = false;
  }

  if (connected == false)
  {
    pClient->disconnect();
    DisplayText("Characteristic UUID(s) not found");
    return false;
  }

  return true;
}

bool connectCharacteristic1(BLERemoteService *pRemoteService, BLERemoteCharacteristic *pRemoteCharacteristic)
{
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  if (pRemoteCharacteristic == nullptr)
  {
    DisplayText("Failed to find our characteristic UUID1");
    return false;
  }
  DisplayText("Found our Characteristic 1");

  // Assign callback functions for the Characteristic(s)
  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback1);

  return true;
}

bool connectCharacteristic2(BLERemoteService *pRemoteService, BLERemoteCharacteristic *pRemoteCharacteristic)
{
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  if (pRemoteCharacteristic == nullptr)
  {
    DisplayText("Failed to find our characteristic UUID2");
    return false;
  }
  DisplayText("Found our Characteristic 2");

  // Assign callback functions for the Characteristic(s)
  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback2);

  return true;
}

// Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    String myDisplay = "BLE Advertised Device found: " + String(advertisedDevice.toString().c_str());
    DisplayText(myDisplay);

    if (advertisedDevice.getName() == bleServerName)
    { // Check if the name of the advertiser matches
      DisplayText("Found the Device and are connecting ...");
      advertisedDevice.getScan()->stop();                             // Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); // Address of advertiser is the one we need
      doConnect = true;                                               // Set indicator, stating that we are ready to connect
    }
  }
};

// When the BLE Server sends a new value reading with the notify property
int currentFrontGear = 0;
int currentRearGear = 0;
static void notifyCallback1(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  // Gears
  // 2 ... # Selected Gear in Front
  // 3 ... # Selected Gear in Rear
  // 4 ... # Gears in Front
  // 5 ... # Gears in Rear

  // String strDebug = "Gearing:" + String(pData[4]) + ":" + String(pData[5]) + "\n" + "Selected Gears:" + String(pData[2]) + ":" + String(pData[3]);
  // DisplayText(strDebug);

  if (currentFrontGear != pData[2] || currentRearGear != pData[3])
  {
    // Draw Graphics
    GearGraph(pData[4], pData[5], pData[2], pData[3]);
    GearText(pData[2], pData[3]);
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

static void notifyCallback2(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  uint16_t tmp16 = (pData[3] << 8 | pData[2]);
  int intPower = (int)(tmp16);
  float flPowerToWeight = (float(intPower)/float(WEIGHT));

  // Button Toggle
  if (bolToggleScreen)
  {
    // Action 1
    PowerText(String(flPowerToWeight, 1));
  }
  else
  {
    // Action 2
    PowerText(String(intPower));
  }
}

void setup()
{
  Serial.begin(250000);

  // Screen Setup
  // tft.init();
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_MAGENTA);

  // Init BLE device
  BLEDevice::init("");
  DisplayText("Initializing Application");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

// Graphics ---------------------------------------------
void GearGraph(int myFrontGears, int myRearGears, int selectedFrontGear, int selectedRearGear)
{
  // Size of Graph
  // #define IWIDTH 190
  #define IWIDTH 190
  #define IHEIGHT RESOLUTION_Y

  sprGearGraph.setColorDepth(16);
  // create sprite with size of screen
  sprGearGraph.createSprite(IWIDTH, IHEIGHT);
  // Background Color
  sprGearGraph.fillSprite(myColorBackground);

  int myGearWidth = (IWIDTH - 35) / (myFrontGears + myRearGears);
  #define myGearHeight (RESOLUTION_Y - 60)
  #define myGearSpacing -1
  #define myGearX 15
  #define myGearY 30

  // FRONT GEARS (CHAINRINGS/CRANKSET)
  for (int i = 0; i < (myFrontGears); i++)
  {
    if (selectedFrontGear == i)
    {
      sprGearGraph.fillRect(myGearX + i * (myGearWidth + myGearSpacing), myGearY + (myFrontGears - i - 1) * (myGearHeight / (myFrontGears * 2)), myGearWidth, (i + 1) * (myGearHeight / myFrontGears), myColorGearSelected);
      sprGearGraph.drawRect(myGearX + i * (myGearWidth + myGearSpacing), myGearY + (myFrontGears - i - 1) * (myGearHeight / (myFrontGears * 2)), myGearWidth, (i + 1) * (myGearHeight / myFrontGears), myColorGearBorder);
    }
    else
    {
      sprGearGraph.drawRect(myGearX + i * (myGearWidth + myGearSpacing), myGearY + (myFrontGears - i - 1) * (myGearHeight / (myFrontGears * 2)), myGearWidth, (i + 1) * (myGearHeight / myFrontGears), myColorGearBorder);
    }
  }

  // REAR GEARS (CASSETTE)
  for (int i = 0; i < (myRearGears); i++)
  {
    if (selectedRearGear == i)
    {
      sprGearGraph.fillRect(20 + (myGearX + (myFrontGears * myGearWidth) + (myFrontGears * myGearSpacing)) + i * (myGearWidth + myGearSpacing), myGearY + i * (myGearHeight / (myRearGears * 2)), myGearWidth, myGearHeight - i * (myGearHeight / myRearGears), myColorGearSelected);
      sprGearGraph.drawRect(20 + (myGearX + (myFrontGears * myGearWidth) + (myFrontGears * myGearSpacing)) + i * (myGearWidth + myGearSpacing), myGearY + i * (myGearHeight / (myRearGears * 2)), myGearWidth, myGearHeight - i * (myGearHeight / myRearGears), myColorGearBorder);
    }
    else
    {
      sprGearGraph.drawRect(20 + (myGearX + (myFrontGears * myGearWidth) + (myFrontGears * myGearSpacing)) + i * (myGearWidth + myGearSpacing), myGearY + i * (myGearHeight / (myRearGears * 2)), myGearWidth, myGearHeight - i * (myGearHeight / myRearGears), myColorGearBorder);
    }
  }

  sprGearGraph.pushSprite(130, 0);
  sprGearGraph.deleteSprite();
}

void GearText(int selectedFrontGear, int selectedRearGear)
{
  // Size of Graph
  #define IWIDTH 130
  #define IHEIGHT (RESOLUTION_Y / 2)

  sprGearText.setColorDepth(16);
  sprGearText.createSprite(IWIDTH, IHEIGHT);
  sprGearText.fillSprite(myColorBgFont);
  // Draw a background
  sprGearText.fillRect(0, 0, IWIDTH, IHEIGHT, myColorBgFont);

  sprGearText.loadFont(digits);
  sprGearText.setTextColor(myColorFont, myColorBgFont);
  sprGearText.setTextDatum(MC_DATUM);
  sprGearText.drawString(String(selectedFrontGear + 1) + ":" + String(selectedRearGear + 1), IWIDTH/2, (IHEIGHT/2)+5);
  sprGearText.unloadFont();

  sprGearText.pushSprite(0, 0);
  sprGearText.deleteSprite();
}

void PowerText(String strPower)
{
  // Size of Graph
  #define IWIDTH 130
  #define IHEIGHT (RESOLUTION_Y / 2)

  sprPowerText.setColorDepth(16);
  sprPowerText.createSprite(IWIDTH, IHEIGHT);
  sprPowerText.fillSprite(myColorBgPower);
  // Draw a background
  sprPowerText.fillRect(0, 0, IWIDTH, IHEIGHT, myColorBgPower);

  sprPowerText.loadFont(digits);
  sprPowerText.setTextColor(myColorFont, myColorBgPower);
  sprPowerText.setTextDatum(MC_DATUM);
  sprPowerText.drawString(strPower, IWIDTH/2, (IHEIGHT/2)+5);
  sprPowerText.unloadFont();

  sprPowerText.pushSprite(0, IHEIGHT);
  sprPowerText.deleteSprite();
}

void loop()
{
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true)
  {
    if (connectToServer(*pServerAddress))
    {
      DisplayText("Connected to BLE Server");
    }
    else
    {
      DisplayText("Failed to connect - Restart to scan");
    }
    doConnect = false;
  }

  // BUTTON ACTION
  int intButtonState = digitalRead(BUTTON_PIN);

  if (intLastButtonState != intButtonState)
  {
    // Debounce Time
    delay(50);

    if (intButtonState == LOW)
    {
      // Press Action
      // Toggle Button State
      bolToggleScreen = !bolToggleScreen;
    }
    else
    {
      // Release Action
    }
    intLastButtonState = intButtonState;
  }
}