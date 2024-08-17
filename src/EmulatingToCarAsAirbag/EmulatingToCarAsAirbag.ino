#define OUTPUT_PIN 32
#define INPUT_PIN 35
 
hw_timer_t *tmr0 = NULL; // 9ms message send
hw_timer_t *tmr1 = NULL; // To send the message
 
uint16_t REQUEST_MESSAGE = 0b010100010; // What to listen from the car
uint16_t NO_OCCUPIED_MESSAGE = 0b100101101; // What to send when no one is sat on the seat
uint16_t SEMI_OCCUPIED_MESSAGE = 0b010101000; // What to send when something is on the seat, but no enought for airbag
uint16_t OCCUPIED_MESSAGE = 0b110000100; //  What to send when someone is sat on the seat

uint8_t txCounter = 0;
uint8_t sentMessagesCounter = 0;
uint16_t messageToSend = 0; // This is used to not change the message mid way if ever needs to be changed

bool rxEnabled = 0;
bool rxReceiving = 0;
uint16_t rxBuffer = 0;
uint32_t rxCounter = 0;

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


// Called by tmr0
void IRAM_ATTR sendMessage()
{
  if (sentMessagesCounter++ < 3)
  {
    txCounter = 0;

    writePin(OUTPUT_PIN, 0); // PULL DOWN FIRST TIME TO BEGIN MESSAGE

    rxEnabled = 0; //Disable RX to avoid confusion

    // Start the timer to output the message
    startTimer(tmr1);
  }
  else
  {
    // Stop and reset the timer
    stopTimer(tmr0);

    // Enable the rx
    rxEnabled = 1;

    // Reset corresponding stats
    rxBuffer = 0;
    rxCounter = 0;
    rxReceiving = 0;
  }

}

// Called by tmr1
void IRAM_ATTR messageBit()
{

  // messageBit is configured to read
  if (rxEnabled)
  {
    rxReceiving = 1;
    switch (rxCounter++)
    {
      case 9: // It has finished reading last bit

        // Stop and reset the timer
        stopTimer(tmr1);

        // Check the data received
        if (rxBuffer == REQUEST_MESSAGE)
        {
          // The message is the right one, respond correspondingly
          // Send 3 times

          // Reset the sent message counter
          sentMessagesCounter = 0;

          // Set the message to send
          // messageToSend = OCCUPIED_MESSAGE; // This will change depending on the conditions
          messageToSend = NO_OCCUPIED_MESSAGE; // This will change depending on the conditions

          startTimer(tmr0);
        }
        else
        {
          // Enable the rx
          rxEnabled = 1;

          // Reset corresponding stats
          rxBuffer = 0;
          rxCounter = 0;
          rxReceiving = 0;
        }

        break;

      case 0:
        rxBuffer |= digitalRead(INPUT_PIN);

      default:
        rxBuffer <<= 1;
        rxBuffer |= digitalRead(INPUT_PIN);
        break;
    }

    return;
  }


  // messageBit is configurede to write
  if (txCounter < 10)
  {
    switch (txCounter++)
    {
      case 9:
        writePin(OUTPUT_PIN, 1);
        break;
      default:
        writePin(OUTPUT_PIN, (messageToSend >> (9-txCounter) & 1));
        break;
    }
  }else // Its finished sending the message
  {
    // Stop and reset the timer
    stopTimer(tmr1);

    // Go back to high state (resting state)
    writePin(OUTPUT_PIN, 1);
  }
}

// If something is read on the pin
void IRAM_ATTR incommingMessage()
{
  if (!rxEnabled) return;
  if (rxReceiving) return;

  delayMicroseconds(45); // Wait to capture in the middle of the wave

  // Start the capturing
  startTimer(tmr1);
}


void setup()
{
  // Serial.begin(115200);

  btStop();

  pinMode(OUTPUT_PIN, OUTPUT);
  writePin(OUTPUT_PIN, 1);

  pinMode(INPUT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INPUT_PIN), incommingMessage, CHANGE);

  tmr0 = timerBegin(0, 80, true);
  timerAttachInterrupt(tmr0, &sendMessage, true);
  timerAlarmWrite(tmr0, 10*1000, true); // 1000 = 1ms
  timerAlarmDisable(tmr0);
  timerStop(tmr0);


  tmr1 = timerBegin(1, 80, true);
  timerAttachInterrupt(tmr1, &messageBit, true);
  timerAlarmWrite(tmr1, 95, true); // 1000 = 1ms
  timerAlarmDisable(tmr1);
  timerStop(tmr1);


  rxEnabled = 1;

}

void loop()
{
}