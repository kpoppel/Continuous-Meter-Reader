[![Build Status](https://travis-ci.org/martinhansdk/Continuous-Meter-Reader.png)](https://travis-ci.org/martinhansdk/Continuous-Meter-Reader)

# Continuous Meter Reader

A little gadget that sits on your water or electricity meter and
monitors it continuously.  The collected reading are presented on a
mobile web page so that you can practice saving ressources by
answering questions like "How much water do I use when I take a
shower?".

Currently Sensus water meters are supported. They look like this:

<img alt="Sensus water meter" src="pcb/Optical water meter reader/sensus_meter.jpg" width="50%">

I have designed two versions of a PCB which fits on top of the water
meter which uses [GP2S700HCP reflective
photointerrupter](http://www.sharp-world.com/products/device/lineup/data/pdf/datasheet/gp2s700hcp_e.pdf)
sensors to read the turning of the half shiny half red disc. The disc
makes one turn per liter. The PCB has six optical sensors, so it can
sense quantities around 1/12 of a liter. The reflective part of the
disc is not exactly a half circle, so the exact 12 quantities measured
are different from each other. An automatic calibration method is
provided which determines how much each quantity is. You can print a
[PDF rendering of the PCB layout](pcb/Optical water meter
reader/optical meter reader board.pdf), make 5mm holes for the
steering pins plus a small hole for the middle of the sensor circle in
your printout and see if it will fit on your meter. When attached to
your meter the small hole should be right above the center of the
disc.

<img alt="Photo of the PCB" src="pcb/Optical water meter reader/pcb lineup.png" width="50%">

* The top row shows the version of the PCB designed to run on two 1.5V batteries and transmit the readings wirelessly.
  * [Eagle schematics](pcb/Optical water meter reader/optical meter reader v3.sch) / [PDF schematics](pcb/Optical water meter reader/optical meter reader v3 battery assembly.pdf)
  * [Eagle board file](pcb/Optical water meter reader/optical meter reader v3.brd)
  * [Gerber files](pcb/Optical water meter reader/optical meter reader v3 - gerber.zip)
  * [Bill of materials](pcb/Optical water meter reader/optical meter reader v3 battery assembly BOM.txt)
  * [Firmware](src/Continuous-Meter-Reader/Continuous-Meter-Reader.ino)
* The bottom row shows the version of the PCB designed to be connected directly to a PC using a USB cable. In the configuration shown here it operates as the radio receiver that receives the readings transmitted from other units.
  * [Eagle schematics](pcb/Optical water meter reader/optical meter reader.sch) / [PDF schematics](pcb/Optical water meter reader/optical meter reader.pdf)
  * [Eagle board file](pcb/Optical water meter reader/optical meter reader.brd)
  * [Gerber files](pcb/Optical water meter reader/optical meter reader gerber.zip)
  * [Firmware](src/RadioStation/RadioStation.ino)

The ambition is that the same or a similar PCB will be able to read
the turning weel on an old style electricity meter. The PCB was
designed in [Eagle pcb](http://www.cadsoftusa.com/) and the schematics
are available [here](pcb/Optical water meter reader/optical meter
reader.pdf).

The USB version uses an [Arduino
Nano](https://www.arduino.cc/en/Main/ArduinoBoardNano) as the CPU
module and [NRF24L01+ Wireless Transceiver
Module](http://www.icstation.com/1pcs-nrf24l0124ghz-wireless-transceiver-module-arduino-p-1388.html)
for wireless connectivity. The battery powered module has the
microcontroller directly on the PCB.

The server application runs on a PC written in
[Go](https://golang.org/) and stores the measurements in a
[Postgres](http://www.postgresql.org) database. The server application
exposes a REST API, a [socket.io](http://socket.io/) service for real
time updates and also serves a web application written in
[React](http://facebook.github.io/react/).

The communication protocol between the embedded code and the server is
based on [Google's protocol
buffers](https://developers.google.com/protocol-buffers/) with the
Arduino implementation generated by
[Nanopb](http://koti.kapsi.fi/jpa/nanopb/). The protocol definition
file is [MeterReader.proto](MeterReader.proto).

## Software build instructions

All build instructions assume Ubuntu 15.04 or later.

### Install prerequisites

    sudo apt-get install make clang protobuf-compiler arduino postgresql-9.4 git
    wget https://godeb.s3.amazonaws.com/godeb-amd64.tar.gz
    tar xzf godeb-amd64.tar.gz
    ./godeb install

### Getting the code

The project uses git submodules, so you have to use a recursive clone to get everything:

    git clone --recursive https://github.com/martinhansdk/Continuous-Meter-Reader

### Uploading the embedded code to the MCU on the battery powered meter reader node

Using an AVR ISP programmer (for instance an Arduino with the
ArduinoISP sketch loaded), attach it to the ISP header on the meter
reader board. In the Arduino IDE select the appropriate programmer
type.  Choose the board type to be "Arduino Pro or Pro Mini (3.3V, 8
MHz) w/ ATmega328". Then run "Tools > Burn bootloader".

Once this has completed correctly, the brown out detection must be
cleared in the AVR in order to allow operating the AVR at 2.5V. Do
this by running:

    avrdude -patmega328p -cstk500v1 -P /dev/ttyUSB? -b19200 -U efuse:w:0xff:m

Now disconnect the ISP and connect a 3.3V FTDI USB to serial adapter to
JP2. The run

    cd src/Continuous-Meter-Reader
    make upload

### Uploading the embedded code to the Arduino on the receiver station
    
Attach the Arduino for the server receiver station and run

    cd src/RadioStation
    make upload
    
### Building the server and node configuration tool

    cd go
    export GOPATH=$PWD
    go get -d
    go build MeterServer.go 
    go build ConfigNode.go 
    go build SampleSender.go 

### Setup the Postgresql database

Drop step 1 if you already have a database running

1. change postgres default password:
   su - postgres
   psql
   \password postgres
   <enter new password>
2. Create the 'meter' user:
   CREATE USER meter;
3. Set user password:
   \password meter;
   <use password: meter2>
3. Create the database:
   CREATE DATABASE meter;
4. Grant priviledges to meter on database:
   GRANT ALL PRIVILEGES ON DATABASE meter TO meter;
5. Grant priviledges to meter on the database tables:
   GRANT ALL PRIVILEGES ON TABLE meters TO meter;
   GRANT ALL PRIVILEGES ON TABLE measurements TO meter;
6. Create the database fields by executing the commands in database.sql
7. Insert the first meter into the meter table:
   INSERT INTO meters(
               id, name, unit, current_series, last_count, scale)
       VALUES (1, 'Mock meter', "L", "0", "0", 1e-3),
              (10,  "Water", "L", "0", "0", 1e-3);
   
### Configuring the Arduino for a meter

Each node has an id which is an integer between 1 and 127. We need to
set this id and also choose the wireless or serial (USB)
protocol. Configuration can only happen over USB.

To configure an Arduino that is attached to /dev/ttyUSB0 run something like the following:

    go/ConfigNode -serial=/dev/ttyUSB0 -wirelessproto -id=10 -uncalibrate
    

### Calibrating a meter sensor

The calibration procedure is as follows:

1. Attach the PCB to the water meter
2. Connect the Arduino to the PC using a USB cable
3. Turn on a tap so that a constant flow is running. Make sure this is the only place water is consumed in the house while the calibration is running
4. Run the command `go/ConfigNode -serial=/dev/ttyUSB0 -calibrate`
5. The command will exit when calibration is complete

### Running the Server

Once the database is set up, and the MeterServer built, it can be run:

   cd go
   ./MeterServer

Contact the server using http://localhost:2111/static

## Running the mock meter (offline mode)

Running tests with out having to flush a lot of water makes sense, so use the

   cd go
   ./SampleSender

for this purpose.

If everything is ok, the example output should look something like this:

    2015/11/10 22:25:00 Waiting for current settings
    2015/11/10 22:25:00 Connection established
    2015/11/10 22:25:00 Waiting for data
    2015/11/10 22:25:02 len= 14
    2015/11/10 22:25:02 n=14
    2015/11/10 22:25:02 NOTE : Received 'Rebooted'
    2015/11/10 22:25:02 Waiting for data
    2015/11/10 22:25:02 len= 24
    2015/11/10 22:25:02 n=24
    2015/11/10 22:25:02 Waiting for data
    2015/11/10 22:25:02 Got current settings
    {"meterId":2,"seriesId":73,"communicationChannel":1,"risingEdgeAmounts":[0,1,2,3,4,5],"fallingEdgeAmounts":[0,1,2,3,4,5]}
    2015/11/10 22:25:02 id=4294967295
    2015/11/10 22:25:57 len= 27
    2015/11/10 22:25:57 n=27
    2015/11/10 22:25:57 Waiting for data
    {"meterId":2,"seriesId":73,"communicationChannel":1,"risingEdgeAmounts":[127,93,117,149,169,153],"fallingEdgeAmounts":[33,15,17,38,41,48]}
