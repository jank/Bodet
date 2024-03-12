# Bodet Flip Clock

I am trying to take an old Bodet style slave flip clock backt to live.

The model I found is an old T&N (Telefonbau & Normalzeit) flip clock. It looks like it was produced by Bodet and only marketed under the TN brand. Model sticker on the inside says BT637 and it looks very similar to this one (https://hackaday.io/project/186457-bodet-flip-clock-hacked-back-to-the-present).

## The Plan

* check the clock is working
  * done: connected it to a 12V power supply. Flipping + and - causes the clock to advance by one minute
* create spare tiles
  * some hour and minute tiles are missing. I am trying to laser cut them from plexiglas
* run clock on Ardunino
  * follow the approach from https://hackaday.io/project/186457-bodet-flip-clock-hacked-back-to-the-present and trigger the minute via an Arduino

## Check the clock is working

This was straight forward. Hooked up the clock to 12V supply and switched + and -. On every switch of polarity the clock advanced by one minute.

## Create spare tiles

I used Fusion 360 to design the tiles (hour and minute tiles are identical for my clock). I will try to laser cut them from plexiglas. DXF file shared in this repository.

### Hard Paper / Epoxy Paper Laminate

I tried cuttin hard paper using a laser cutter. This did not produce the requried results. The laser cutter could not cut out the small, 1mm wide, structures of the flip. Even varying the speed or maximum intensitiy of the laser did not produce better reults. The material was also too hard to cut it with a Cricut Air 2.

### PVC Plastic

I found a rigid-PVC material in black with 0.3 mm thickness in an online shop: https://www.modulor.de/en/rigid-pvc-opaque-coloured.html

While the material is a bit more flexible than the original material it worked out well. Cutting with the Cricut Air 2 worked pretty well.

### Putting numbers on the spare tiles.

On the internet I found the font 'folio' as recommendation for Bodet flip tiles. Next up is cutting out numbers from a thin white foil and attach them to the spare tiles. 

## Control the Slave Clock

Due to the lack of a matching master clock, I want to run my slave clock using a microcontroller. Based on this [project](https://hackaday.io/project/186457-bodet-flip-clock-hacked-back-to-the-present) I purchased the following equipment for version 1:

* AZ-ATmega328DIP-Board Mikrocontroller Board ATmega16U2 8-bit Dev Board
*	Jumper Wire cables M2M/ F2M / F2F 
* Prototyping Prototype Shield Mini Breadboard for UNO R3 (came with bundle)
* Double H-Bridge DC Motor Controller Board Modul AZ-L298N
* 220V AC/DC to 12V DC Mini converter
