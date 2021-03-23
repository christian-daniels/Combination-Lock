# Combination-Lock
An embedded system that emulates a simple combination lock. Using an ATMega32 microcontroller, a keypad as input, and a LCD and speaker as output a user is able to unlock and lock the device.

## Functions
### Lock Device
The system first takes 4 numbers to lock the device. A user is asked to verify their lock.
### Unlock Device
A user may unlock the system by entering the 4 numbers previously used to lock the device. When the user guesses the combination incorrectly three consecutive times they are locked out of the system for 60 seconds.
### Manager Mode
In the instance a user forgets their password they may bring the device in to a manufacturer where a manager will input the global combo reset to manually unlock the device for the user.

## Main Modules Used
* ATMega32 Microcontroller
* 16 input Keypad
* 16 Character LCD
* Speaker
