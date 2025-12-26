# kite
Sam's Windmill and Panic switch system

Special Thanks to Jonathan Appavoo for design and creation.

There are three programs: two test programs and one arudino program

The purpose of the prgram is to run the arudino and ancillary circuit to operate the inspeed hall effect annenometer and control a remote switch. The switch will control a 120 volt plug that is intended to control a relay to turn the ancillary AC dump load on for the wind turbine. The relay will keep the relay on the dumpload with no power and the dumpload off with power.  The conterol box has green light that will turn off once a revolution of the anenometer.  The red light will be activated if the panic function is activated.

The input for the arduiono control are three wires from the annenometer. Red is  volt in and Green is ground and the white is the singal wire(about 2 volts out).

The output is red andd green to the remote switch.

The arduino box takes the voltage to 5volts for the sense pin 8. Once a revolution of the annenometer the voltage will drop to 5 volts. The arduino will track this with time and estimate speed. It derive current speed, average speed and max speed and estimate gust speed.
The panic button will be activated (turning the remote 120 volt swich to off) when a set max speed (in the program code)or gust speed is ex exceeded. The name of aruduino program is Kite/ino.

Twp test programs exist. The Panic Test Program switch will test the panic test switch function.  The other program is the Sensor Test Program wich will test the input of the system.
 
General comments for sketch book window:  The check mark compiles the program. The right arrow key sends program to the arduino. The program starts running as soon as it is uploaded. To observe the output click the search symbol.

Samuel Appavoo
26/12/2025

