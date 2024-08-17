# 350z - Airbag OCS

This repo is about how I fixed a issue related to the airbag of the Nissan 350z

## Context

The Nissan 350z has a SRS (Supplementary Restrain System) as every other car on the road. However, in this vehicle, you can't disable the passenger airbag at will, it is the car the one who decides whether to activate it or not. And because of that, I'm writing this. 😐

In my case, the OCS (Occupant Classification System) is the one that failed. And "What is the OCS?" I heard you ask. Well, the OCS is a sensor embedded in the passenger seat cushion. This system is only available from 2006 cars onwards, and because I have a 2006 coupé, I have it.

The OCS is in charge of determining the passenger weight and tell the car whats sat on the seat. It can be 3 possibilities:

- A person (Airbag will deploy)
- Groceries, bags... (Airbag will not deploy)
- Nothing (Airbag will not deplay)

So it's only when there's someone sat there will the airbag deploy. And becuase my sensor is broken, I no longer have any airbag on the passenger.

The specific issue here is the behavour:

- When there is someone sat --> Send "error"
- When there is no one sat --> Send "no one sat"

So to address the issue, all I have to do is tell the car "someone is sat" when I receive an "error" from the sensor, and tell the car "no one is sat" when I get the "no one is sat" from the sensor. Easy enough...

> **NOTE:** I want to preserve the car's features as long as posible, I don't want to tell the car that there's someone sat there when there isn't because the seatbelt light would be on all the time. Annoying.

## Solution

A board, of course, that will act based on the sensor output, but I'm going to be able to override the output with an app I made for the car stereo.

How the board works is just easy, you only need to emulate to the sensor as the car, and emulate to the car as the sensor. Try to say that 3 times fast...

## What's available in this repo?

In this repo you will find the full code [with](/src/Airbag_OCS_Dummy_BLE/) and [without](/src/Airbag_OCS_Dummy/) BLE control. The only thing left if you want to get this working is to step up the logic from 3v3 to 12v for the car to understand.

Also, you will find the .sal files that I got from the logic analyzer to get how the protocol works and the individual emulation files ([emulate to the car as the sensor](/src/EmulatingToCarAsAirbag/), and [emulate to the sensor as the car](/src/EmulatingToAirbagAsCar/))

## Picture of the board

Yeah, it's missing the ESP32, I soldered it after taking the photo.

> **NOTE:** The SEMIOCCUPIED_SWITCH_PIN is just a useless switch having the BLE code

![Board](/assets/Board.jpg)
