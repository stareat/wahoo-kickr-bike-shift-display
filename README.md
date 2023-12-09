# External Display for WAHOO KICKR BIKE SHIFT
Code is written for LILYGO T-Display-S3 and displays the following Bluetooth Low Energy (BLE) data:

- Gearing ratio (as set in the wahoo app) and what gears you are currently in. Information (text and graphic) will update when shifting.
- Cycling power measurement (Watt or Watts Per Kilogram). Displayed information can be changed via a button (BOOT) on the device. (The rider weight must be set in the code!)

![alt text](wahoo-kickr-bike-shift-display.jpeg)

## Setup
You need to install the Arduino IDE to compile and upload the code to the microcontroller. A very good HOWTO can be found here:
https://github.com/teastainGit/LilyGO-T-display-S3-setup-and-examples/blob/main/T-DisplayS3_Setup.txt

## A few things to consider

1. If you have changed the Bluetooth name of your bike (in the wahoo app) you will need to change it also in the code:

> // BLE Server name
> 
> #define bleServerName "KICKR BIKE SHIFT 720C"


2. If you want to use the power-to-weight ratio you must set your weight (kg) in the code:

> #define WEIGHT 75


3. The graphic should scale depending on the gearing setting, so it will fit the screen. (There are variables to change the rendering if necessary.)

> int myGearWidth = (IWIDTH - 35) / (myFrontGears + myRearGears);
> 
> #define myGearHeight (RESOLUTION_Y - 60)
> 
> #define myGearSpacing -1
> 
> #define myGearX 15
> 
> #define myGearY 30


4. Debugging: All Status messages during initiation and during connecting process will be displayed on the TFT and in the serial monitor window of the IDE. I tried to comment the code so that it is easier to understand.

5. To identify BLE GATT servies and characteristics use https://www.bluetooth.com/specifications/assigned-numbers/ and if you look at GATT Specification Supplement 5 it describes how to intepret the bytes: https://www.bluetooth.com/specifications/specs/gatt-specification-supplement-5/
   
7. Scan BLE devices: https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-mobile
