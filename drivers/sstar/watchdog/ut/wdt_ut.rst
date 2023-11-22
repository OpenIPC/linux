======================================================
wdt_ut.rst is the guide to use sigmastar wdt_ut demo
======================================================
How to get the wdt_ut:
Just use "Make" command, it will compile the wdt_ut.c into wdt_ut.

How to use the wdt_ut:
(1) Start the wdt
./wdt_ut start [timeout(s)]
start the watchdog, and it will keep it alive,default timeout is 5s.
(2) Stop the wdt
ctrl + c
(3) Reset the chip
./wdt_ut reset [timeout(s)]
start the watchdog, but not to keep it alive,after the timeout time, it will reset the chip.
