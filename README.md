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

I tried cuttin hard paper using a laser cutter. This did not produce the requried results. The laser cutter could not cut out the small, 1mm wide, structures of the flip. Even varying the speed or maximum intensitiy of the laser did not produce better reults.

Next up is trying to cut the hard paper with a Cricut.
