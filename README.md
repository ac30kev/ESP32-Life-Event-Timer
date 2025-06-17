# ESP32-Life-Event-Timer
Using a ESP32 2432S028R to create a 'countdown to target date' timer.

The idea is to have a nice colourful timer to let you know how long until a certain date. My original intention was to countdown to my retirement, but then I realised that it could be used to countdown to my next holiday abroad, or my mates wedding, or, hey, what about... you get the idea.
I want to be able to add, remove or edit target date info, (including time in HH:MM format) because these things tend to go past rather quickly.
I also wanted to edit the wifi credentials stored within, as I may take it to another place than home or work, so I can use direct touch screen to input the data.
I've done most of the work, but need git to help my iteration control because its dreadful and I keep getting lost...

Also, I stole ( and altered) the step files for an excellent case from https://www.thingiverse.com/thing:6662492. 
Genius work but better now as you can store the stylus with the ESP32. And space for the speaker (speaker? ...yes.) and battery storage and charging device placement.
These I will upload at a later date as I'm struggling to do so now.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

New amendments: added the 2 codes for flashing the alarm for the 'teas ready' alarm; 1 for cyd extra ino file, and one for the esp32.
I now want to add code for the 'breath' cube to also flash bright red when the pushbutton is activated.

The MAC codes need watching.

I have up-issued all codes, latest being MyTimer-v5.0.0, (inc BellOverride), BellPush1.7 and BellAndBreath1.0

just need to add the MAC address, which should show on Serial.print, when uploaded.
