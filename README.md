# Whirlwind

This is a RP2040 firmware for the open source PC to Arcade Cabinet Interface designed by Barito Nomarchetto.

This is supposed to be my improvement of the initial version https://github.com/baritonomarchetto/Whirlwind/blob/main/whirlwind.ino.

I tried to orientate on the concepts of the Deamon Bite Arcad Encoder
https://github.com/MickGyver/DaemonBite-Arcade-Encoder/blob/master/DaemonBiteArcadeEncoder/DaemonBiteArcadeEncoder.ino

The complete original project can be found here https://www.instructables.com/Whirlwind-PC-to-JAMMA-Arcade-Cabinet-Interface/

This program reads button states from an arcade controller and simulates keyboard presses accordingly. 
It also provides optional frequency blocking functionality.

Dependencies: This program depends on the Keyboard and pico/stdlib libraries.
It is designed to work with the RP2040 microcontroller.

Button and shift functionality are handled separately. Each button can have a normal and a shifted function, much like a real keyboard.

Button debouncing is implemented to avoid multiple inputs from a single press.

When SYNC_MONITOR_ACTIVE is defined, the frequency block functionality is enabled.

This checks the frequency of the horizontal sync pin and enables or disables the video amp and LED accordingly.

