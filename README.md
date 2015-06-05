#Tripolar
Tripolar stepper motor driver firmware designed for Atmega8 based ESCs and 3-phase BLDC motors. Three-phase sensorless brushless motor drivers are inexpensive and ubiquitous in RC/robotics industry. Recently, three-phase "gimbal" motors have become common and rely on high turn-count motors and stepper motor control to produce high-torque, precisely controlled movement.

Additionally, there are many applications that require low-speed, high-torque movement, such as wheels on robots. The back-EMF position sensing method used on typical brushless motors is not able to operate at very low speeds and can't be used in these applications.

"Tripolar" aims to achieve the following:

* Provide a low-speed, high-torque driver for three phase brushless motors
* Long-term, we'd like to blend this control method with traditional back-EMF feedback control to provide smooth control from 0 rpm. (Possibly patented by Bosch. See Hackaday comments on slooowwww BLDC.)

##Warning - Not For Use With All Motors

This firmware runs the brushless motor at high currents for maximum torque. This will quickly overheat and burn out a traditional brushless motor designed for high power outputs. *This firmware should only be used with "gimbal" style brushless motors with phase resistance of 3.5 ohms or greater.*

##Toolchain

Use of this firmware requires a copy of Arduino (ideally Arduino 0022), as well as avr-gcc, cmake, and avrdude.

##Compilation

This code is written to be Arduino-compatible to make it as easy as possible to compile and flash ESCs.

```bash
mkdir build
cd build
cmake ../src/ -i
```

You will be presented with several options. Change the Arduino directory to match that on your computer. Currently compatible with Arduino < 1.0.

```bash
cmake ../src/
make
```

The makefile is not yet set up to flash through the tgylinker bootloader. Please follow the instructions below to flash.

##Firmware Flashing

The firmware is normally flashed using the "Turnigy USB Linker" bootloader, which is already present on most ESCs with "SimonK" firmware.

```bash
avrdude -c stk500v2 -b 19200 -P /dev/tty.usbmodemfd131 -p m8 -U flash:w:tripolar.hex:i
```

##Concept

The driver operates by electrifying each of the three motor phases in sequence with a phase delay that determines rotational speed.

Features:

* PWM power output to allow operation at <100% duty cycle
* Delta Sigma Modulator?

##Code Structure

* A timer interrupt running very fast handles PWM generation
	* Interrupt at max clock rate
* Main loop handles phase timing

##Observations From Existing Gimbal Firmware

* FETs are all shut off simultaneously, but turn on at different times
* Delay between high side on and low side off is 5 &mu;s. It looks like this could be reduced to about 2 &mu;s safely
* On time for high side FETs is about 1.5 &mu;s (unconnected), 4 &mu;s (connected)
* Off time for high side FETs is about 1 &mu;s
* Off time for low side FETs is about 1.5 &mu;s

##Test Cases to Start With

1. Pulse pin at full clock rate
2. Pulse pin at 8kHz-18kHz from 0-100% duty cycle
3. Pulse pin with phase delay
4. Pulse three pins with phase delay
5. Test MOSFET on/off functions