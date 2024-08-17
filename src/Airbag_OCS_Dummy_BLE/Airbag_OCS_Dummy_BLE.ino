#define OUTPUT_CAR_PIN 14
#define INPUT_CAR_PIN 12
#define OUTPUT_SENSOR_PIN 27
#define INPUT_SENSOR_PIN 35

#define SEMIOCCUPIED_SWITCH_PIN 33

#define LED_PIN 26

#include "BLEHandler.h"


enum blinkPattern
{
  NO_OCCUPIED_BLINK = 0b00011000,
  SEMI_OCCUPIED_BLINK = 0b01010101,
  OCCUPIED_BLINK = 0b11001100,
};

uint8_t ledBlinkPattern = NO_OCCUPIED_BLINK;


// DOCUMENTED ON LOGIC ANALYZER FILES
uint16_t REQUEST_MESSAGE = 0b010100010; // What to listen from the car
uint16_t NO_OCCUPIED_MESSAGE = 0b100101101; // What to send when no one is sat on the seat
uint16_t SEMI_OCCUPIED_MESSAGE = 0b010101000; // What to send when something is on the seat, but no enought for airbag
uint16_t OCCUPIED_MESSAGE = 0b110000100; //  What to send when someone is sat on the seat

uint16_t messageToSend = NO_OCCUPIED_MESSAGE;

// Respond to car
hw_timer_t *tmr0 = NULL; // 9ms message send
hw_timer_t *tmr1 = NULL; // To send the message

uint8_t txCounter_c = 0;
uint8_t sentMessagesCounter_c = 0;
uint16_t messageToSend_c = 0; // This is used to not change the message mid way if ever needs to be changed

bool rxEnabled_c = 0;
bool rxReceiving_c = 0; // To avoid interrupts restarting the timer
uint16_t rxBuffer_c = 0;
uint32_t rxCounter_c = 0;


// Ask the sensor
hw_timer_t *tmr2 = NULL; // 1000ms message send
hw_timer_t *tmr3 = NULL; // To send the message

uint8_t txCounter_s = 0;

bool rxEnabled_s = 0;
bool rxReceiving_s = 0; // To avoid interrupts restarting the timer
uint16_t rxBuffer_s = 0;
uint32_t rxCounter_s = 0;

void writePin(uint8_t pin, bool value)
{
  digitalWrite(pin, !value);
}

void startTimer(hw_timer_t *tmr)
{
  timerWrite(tmr, 0);
  timerAlarmEnable(tmr);
  timerStart(tmr);
}

void stopTimer(hw_timer_t *tmr)
{
  timerAlarmDisable(tmr);
  timerStop(tmr);
}


// Called by tmr0 (Core 1, CAR)
void IRAM_ATTR sendMessage_c()
{
  if (sentMessagesCounter_c++ < 3)
  {
    txCounter_c = 0;

    writePin(OUTPUT_CAR_PIN, 0); // PULL DOWN FIRST TIME TO BEGIN MESSAGE

    rxEnabled_c = 0; //Disable RX to avoid confusion

    // Start the timer to output the message
    startTimer(tmr1);
  }
  else
  {
    // Stop and reset the timer
    stopTimer(tmr0);

    // Enable the rx
    rxEnabled_c = 1;

    // Reset corresponding stats
    rxBuffer_c = 0;
    rxCounter_c = 0;
    rxReceiving_c = 0;
  }

}

// Called by tmr1 (Core 1, CAR)
void IRAM_ATTR messageBit_c()
{

  // messageBit is configured to read
  if (rxEnabled_c)
  {
    rxReceiving_c = 1;
    switch (rxCounter_c++)
    {
      case 9: // It has finished reading last bit

        // Stop and reset the timer
        stopTimer(tmr1);

        // Check the data received
        if (rxBuffer_c == REQUEST_MESSAGE)
        {
          // The message is the right one, respond correspondingly
          // Send 3 times

          // Reset the sent message counter
          sentMessagesCounter_c = 0;

          // Set the message to send
          messageToSend_c = messageToSend; // This will change depending on the conditions

          startTimer(tmr0);
        }
        else
        {
          // Enable the rx
          rxEnabled_c = 1;

          // Reset corresponding stats
          rxBuffer_c = 0;
          rxCounter_c = 0;
          rxReceiving_c = 0;
        }

        break;

      case 0:
        rxBuffer_c |= digitalRead(INPUT_CAR_PIN);

      default:
        rxBuffer_c <<= 1;
        rxBuffer_c |= digitalRead(INPUT_CAR_PIN);
        break;
    }

    return;
  }


  // messageBit is configurede to write
  if (txCounter_c < 10)
  {
    switch (txCounter_c++)
    {
      case 9:
        writePin(OUTPUT_CAR_PIN, 1);
        break;
      default:
        writePin(OUTPUT_CAR_PIN, (messageToSend_c >> (9-txCounter_c) & 1));
        break;
    }
  }else // Its finished sending the message
  {
    // Stop and reset the timer
    stopTimer(tmr1);

    // Go back to high state (resting state)
    writePin(OUTPUT_CAR_PIN, 1);
  }
}

// Called by tmr2 (Core 0, SENSOR)
void IRAM_ATTR sendMessage_s()
{

  // If the semi-occupied switch is flipped, dont even ask (1 - flipped, 0 - non flipped)
  if (digitalRead(SEMIOCCUPIED_SWITCH_PIN))
  {
    // What to send to car, and the blink pattern
    messageToSend = SEMI_OCCUPIED_MESSAGE;
    ledBlinkPattern = SEMI_OCCUPIED_BLINK;
    return;
  }

  // If bluetooth is set to override, dont event ask (see BLEHandler.h)
  if (overrideState) // Different than 0, which is do not override
  {
    switch (overrideState)
    {
      case NO_OCCUPIED_OVERRIDE:
        if (messageToSend != NO_OCCUPIED_MESSAGE || ledBlinkPattern != NO_OCCUPIED_BLINK)
        {
          messageToSend = NO_OCCUPIED_MESSAGE;
          ledBlinkPattern = NO_OCCUPIED_BLINK;
        }
        break;
      case SEMI_OCCUPIED_OVERRIDE:
        if (messageToSend != SEMI_OCCUPIED_MESSAGE || ledBlinkPattern != SEMI_OCCUPIED_BLINK)
        {
          messageToSend = SEMI_OCCUPIED_MESSAGE;
          ledBlinkPattern = SEMI_OCCUPIED_BLINK;
        }
        break;
      case OCCUPIED_OVERRIDE:
        if (messageToSend != OCCUPIED_MESSAGE || ledBlinkPattern != OCCUPIED_BLINK)
        {
          messageToSend = OCCUPIED_MESSAGE;
          ledBlinkPattern = OCCUPIED_BLINK;
        }
        break;
    }

    return;
  }


  txCounter_s = 0;

  // else, write to the sensor and parse the response

  writePin(OUTPUT_SENSOR_PIN, 0); // PULL DOWN FIRST TIME TO BEGIN MESSAGE

  rxEnabled_s = 0; //Disable RX to avoid confusion


  // Start the timer to output the message
  startTimer(tmr3);
}

// Called bt tmr3 (Core 0, SENSOR)
void IRAM_ATTR messageBit_s()
{

  // messageBit is configured to read
  if (rxEnabled_s)
  {
    rxReceiving_s = 1;
    switch (rxCounter_s++)
    {
      case 9: // It has finished reading last bit

        // Stop and reset the timer
        stopTimer(tmr3);

        rxBuffer_s &= 0x1FF;

        // Check the data received 
        if (rxBuffer_s == NO_OCCUPIED_MESSAGE )
        {
          // If there is no one sat, send that code
          messageToSend = NO_OCCUPIED_MESSAGE;
          // Set the blink pattern
          ledBlinkPattern = NO_OCCUPIED_BLINK;
        }
        else
        {
          // If is different, its because someone is sat
          messageToSend = OCCUPIED_MESSAGE;
          // Set the blink pattern
          ledBlinkPattern = OCCUPIED_BLINK;
        }


        rxEnabled_s = 0; // Stop receiving

        break;

      case 0:
        rxBuffer_s |= digitalRead(INPUT_SENSOR_PIN);

      default:
        rxBuffer_s <<= 1;
        rxBuffer_s |= digitalRead(INPUT_SENSOR_PIN);
        break;
    }

    return;
  }


  // messageBit is configured to write the request message
  if (txCounter_s < 10)
  {
    switch (txCounter_s++)
    {
      case 9:
        writePin(OUTPUT_SENSOR_PIN, 1);
        break;
      default:
        writePin(OUTPUT_SENSOR_PIN, (REQUEST_MESSAGE >> (9-txCounter_s) & 0b1));
        break;
    }
  }else // Its finished sending the message
  {
    // Stop and reset the timer
    stopTimer(tmr3);
    writePin(OUTPUT_SENSOR_PIN, 1);

    // Enable the rx
    rxEnabled_s = 1;

    // Reset corresponding stats
    rxBuffer_s = 0;
    rxCounter_s = 0;
    rxReceiving_s = 0;
  }
}



// If something is read on the pin
void IRAM_ATTR incommingMessage_c()
{
  if (!rxEnabled_c) return;
  if (rxReceiving_c) return;

  delayMicroseconds(45); // Wait to capture in the middle of the wave

  // Start the capturing
  startTimer(tmr1);
}

// If something is read on the pin
void IRAM_ATTR incommingMessage_s()
{
  if (!rxEnabled_s) return;
  if (rxReceiving_s) return;

  delayMicroseconds(45); // Wait to capture in the middle of the wave

  // Start the capturing
  startTimer(tmr3);
}


// RUNS ON CORE 0, setups the interrupts to be handled by that core
// Responds to the car
void setupCar(void * parameter)
{

  Serial.print("Car comms running on core ");
  Serial.println(xPortGetCoreID());

  pinMode(OUTPUT_CAR_PIN, OUTPUT);
  writePin(OUTPUT_CAR_PIN, 1);

  pinMode(INPUT_CAR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INPUT_CAR_PIN), incommingMessage_c, FALLING);

  tmr0 = timerBegin(0, 80, true);
  timerAttachInterrupt(tmr0, &sendMessage_c, true);
  timerAlarmWrite(tmr0, 10*1000, true); // 1000 = 1ms
  timerAlarmDisable(tmr0);
  timerStop(tmr0);


  tmr1 = timerBegin(1, 80, true);
  timerAttachInterrupt(tmr1, &messageBit_c, true);
  timerAlarmWrite(tmr1, 95, true); // 1000 = 1ms
  timerAlarmDisable(tmr1);
  timerStop(tmr1);


  rxEnabled_c = 1;

  vTaskDelete(NULL);
}

// RUNS ON CORE 1, setups the interrupts to be handled by that core
// Does: Talk to sensor and get switches states
void setupSensor(void * parameter)
{
  Serial.print("Sensor comms running on core ");
  Serial.println(xPortGetCoreID()); // Normally Core 1

  pinMode(OUTPUT_SENSOR_PIN, OUTPUT);
  writePin(OUTPUT_SENSOR_PIN, 1);

  pinMode(INPUT_SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INPUT_SENSOR_PIN), incommingMessage_s, FALLING);

  pinMode(SEMIOCCUPIED_SWITCH_PIN, INPUT);

  tmr2 = timerBegin(2, 80, true);
  timerAttachInterrupt(tmr2, &sendMessage_s, true);
  timerAlarmWrite(tmr2, 500*1000, true); // 1000 = 1ms // Send a message to sensor evey 500 ms
  timerAlarmEnable(tmr2);


  tmr3 = timerBegin(3, 80, true);
  timerAttachInterrupt(tmr3, &messageBit_s, true);
  timerAlarmWrite(tmr3, 95, true); // 1000 = 1ms
  timerAlarmDisable(tmr3);
  timerStop(tmr3);

  rxEnabled_s = 0;

  vTaskDelete(NULL);
}


////////////////////////////////////////////////////////////////
// CORE 0: Respond to car with the desired message
// CORE 1: Ask the sensor and switches to set that desired message
////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);
  Serial.print("Main code running on core ");
  Serial.println(xPortGetCoreID()); // Normally Core 1

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // btStop();

  // Setup the interrupts on Core 0 to be handled by that core
  xTaskCreatePinnedToCore(
    setupCar, /* Function to implement the task */
    "Car comms setup", /* Name of the task */
    100000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    0,  /* Priority of the task */
    NULL,  /* Task handle. */
    0); /* Core where the task should run */

  
  // Setup the interrupts on Core 1 to be handled by that core
  xTaskCreatePinnedToCore(
    setupSensor, /* Function to implement the task */
    "Sensor comms setup", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    1,  /* Priority of the task */
    NULL,  /* Task handle. */
    1); /* Core where the task should run */

  xTaskCreatePinnedToCore(
    setupBLE, /* Function to implement the task */
    "BLE setup", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    1,  /* Priority of the task */
    NULL,  /* Task handle. */
    1); /* Core where the task should run */

}

void loop()
{
  for (int i = 0; i<8; i++)
  {
    digitalWrite(LED_PIN, (ledBlinkPattern >> (7-i)) & 1);
    delay(125);
  }

}