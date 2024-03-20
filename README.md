# Bodet / T&N Flip Clock

I found a T&N (Telefonbau & Normalzeit) flip clock from the late 70ies/early 80ies. The clock is branded as T&N but was produced by Bodet.The model sticker on the inside says BT637.

I was inspired by this [Hackaday project](https://hackaday.io/project/186457-bodet-flip-clock-hacked-back-to-the-present) by iSax.

This GitHub project documents the process to rebuild my flip clock and provices access to all design files and code created along the way.

## The Plan

[x] check the clock is working: I connected the clock to a 12V power supply. Flipping + and - caused the clock to advance by one minute. So I considered it fully functional.

[x] create spare tiles: I experimented with hard paper and plexiglas. In the end a rigid PVC material worked best. Cut out tiles using a Cricut.

[x] program micro controller as clock driver: the code is ready. I added support for timezone and DST changes. When changing to DST, the clock will advance by 60 minutes, when changing to normal time, the clock will delay an hour.

[ ] solder all parts together: Provide power from 230V AC, wire up and solder 230V AC to 12V DC converter, ESP32, and H-Bridge to drive clock.


#### Check the clock is working

This was straight forward. Hooked up the clock to 12V supply and switched + and -. When the direction of the current was changed (flip +/-) the clock advanced by one minute.

The mechanics to drive the hour from the minute, the weekday and day of month from the hour, and the month from the day of month all worked. I did not check the leap year function. Will do that once all is wired up.

Seven of the hour and two of the minute tiles were missing.

#### Create spare tiles

I first tried to get spare tiles from Bodet. To no surprise, they do not have them on stock anymore.

I used Fusion360 to design the tiles (hour and minute tiles are identical for my clock). You can find the files in the design folder.

Finding the right material for the tiles was a bit of a challenge. I found a local company, [Hema Kunststoff GmbH](https://maps.app.goo.gl/DWzyLc1jadHfFmLP6), specialized in milling plastic materials. They were very helpful in identifying the material: hard paper. It is a fiber composite material made of paper and a phenol-formaldehyde resin (phenolic resin).

I tried laser cutting the hard paper in a local maker space. Unfortunately, this did not work out. The power required to cut the material made it impossible to cut out the fine structure required to slot the tile into the clock wheel. This part is only 1mm thick. 

Next I tried to cut the tiles with a Cricut Air 2. But the hard paper was too rigid and the Cricut did not manage to cut through.

I asked the experts at Hema again. They would have milled the pieces for me, but given the required work hours, it would have cost around 10€ per tile.

After more research on the internet, I ordered the [rigid-PVC](https://www.modulor.de/en/rigid-pvc-opaque-coloured.html) from Modulor. This is a black, 0.3 mm thick material. It turned out to have similar flexibility/stiffness as the hard paper and I was able to cut it with the Cricut.

#### Putting numbers on the spare tiles.

On the internet I found the font 'folio' as recommendation for Bodet flip tiles. Doing test cuts, it quickly became apparent, that none of the myriads of fonts will do. The numbers of hours and minutes are of different size and slightly different shape. I have the impression, that these printing templates were rather hand-crafted based on a sans-serif template.

My wife helped me out here. She scanned tile that had parts of the missing numbers and reconstructed the shape in the Cricut design program. This required a lot of patience and I am happy that she took over.

We neded up picking white adhesive labels as the material to cut out the numbers. The result is quite impressive. From about a meter distance, it is impossible to distinguish the original tiles from the spare tiles.

## Control the Slave Clock

Due to the lack of a matching master clock, I want to run my slave clock using a microcontroller. 

My rough idea for the algorithm was:

* setup()
  * connect to wifi
  * connect to NTP server to keep system clock in sync


* loop() (e.g. every second)
  * check if a minute passed - remember result
  * check if DST switch happend, if yes, add or subtract 60 from tile buffer
  * if minute passed & tile buffer = 0
    * advance clock
  * if minute passed & tile buffer < 0
    * tile buffer++ // the wall clock is ahead of system time
  * if tile buffer > 0
    * advance clock // the wall clock is behind system time

Based on this [project](https://hackaday.io/project/186457-bodet-flip-clock-hacked-back-to-the-present) I purchased the following equipment for version 1:

* AZ-ATmega328DIP-Board Mikrocontroller Board ATmega16U2 8-bit Dev Board
* Prototyping Prototype Shield Mini Breadboard for UNO R3 (came with bundle)
* Jumper Wire cables M2M/ F2M / F2F 
* Double H-Bridge DC Motor Controller Board Modul AZ-L298N
* 220V AC/DC to 12V DC Mini converter

I was intruiged by the capability to run the board on 12V. That is also the current required to drive the clock motor and the H-Bridge. But I must have been a bit tired when I hit purchase. The ATmega board does not have WiFi on-board. Instead of buying a WiFi shield or a real-time-clock I decided to change to this option:

* ESP32 NodeMCU Module WLAN WiFi Development Board | Dev Kit C V2
* Jumper Wire cables M2M/ F2M / F2F 

Luckily, the H-Bridge provides a 5V power out pin. So I can connect the DC Mini converter to the H-Bridge and power the ESP32 from the 5V output. No need for an additional step-down component.