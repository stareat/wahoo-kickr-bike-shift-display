# wahoo-kickr-bike-shift-display
Display Wahoo KICKR BIKE SHIFT Information (BLE/LILYGO T-Display-S3)

- Takes the front and rear setting from the BLE information to draw the initial graph.
- Updates the display (text and graph) when shifting


You need to install the Arduino IDE to compile and upload the code to the microcontroller. A very good HOWTO can be found here:
https://github.com/teastainGit/LilyGO-T-display-S3-setup-and-examples/blob/main/T-DisplayS3_Setup.txt

My code will read the gearing ratio (as set in the wahoo app) from the BLE signal and then draw the graph accordingly. 

A few things to consider:

1. If you have changed the Bluetooth name of your bike (again in the wahoo app) you will need to change it also in the code:

> // BLE Server name
> 
> #define bleServerName "KICKR BIKE SHIFT 720C"

2. If you have a lot of gears e.g. 3x13 then you need to make the displayed gears smaller otherwise they won’t fit on the screen. There are variables (width & spacing) to do so:

> #define myGearWidth 12
> 
> #define myGearHeight 110
> 
> #define myGearSpacing -1
> 
> #define myGearX 140
> 
> #define myGearY 30

4. Debugging: All Status messages during initiation and during connecting process will be displayed on the TFT and in the serial monitor window of the IDE. I tried to comment the code so that it is easier to understand.

5. To identify BLE GATT servies and characteristics use https://www.bluetooth.com/specifications/assigned-numbers/ and/or https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-mobile
