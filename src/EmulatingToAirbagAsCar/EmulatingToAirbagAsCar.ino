#define OUTPUT_PIN 32
#define INPUT_PIN 35
 
hw_timer_t *tmr0 = NULL; // 200ms message send
hw_timer_t *tmr1 = NULL; // To send the message
 
uint16_t message = 0b010100010;
uint8_t txCounter = 0;

bool rxEnabled = 0;
bool rxReceiving = 0;
uint16_t rxBuffer = 0;
uint32_t rxCounter = 0;

void writePin(uint8_t pin, bool value)
{
  digitalWrite(pin, !value);
}

void IRAM_ATTR sendMessage()
{
  txCounter = 0;

  writePin(OUTPUT_PIN, 0); // PULL DOWN FIRST TIME TO BEGIN MESSAGE

  rxEnabled = 0; //Disable RX to avoid confusion


  // Start the timer to output the message
  timerWrite(tmr1, 0);
  timerAlarmEnable(tmr1);
  timerStart(tmr1);

}

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
        timerAlarmDisable(tmr1);
        timerStop(tmr1);

        rxEnabled = 0; // Stop receiving

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
        writePin(OUTPUT_PIN, (message >> (9-txCounter) & 0b1));
        break;
    }
  }else // Its finished sending the message
  {
    // Stop and reset the timer
    timerAlarmDisable(tmr1);
    timerStop(tmr1);
    writePin(OUTPUT_PIN, 1);

    // Enable the rx
    rxEnabled = 1;

    // Reset corresponding stats
    rxBuffer = 0;
    rxCounter = 0;
    rxReceiving = 0;
  }
}

// If something is read on the pin
void IRAM_ATTR incommingMessage()
{
  if (!rxEnabled) return;
  if (rxReceiving) return;

  delayMicroseconds(45); // Wait to capture in the middle of the wave

  // Start the capturing
  timerWrite(tmr1, 0);
  timerAlarmEnable(tmr1);
  timerStart(tmr1);

}


/*
void IRAM_ATTR incommingMessage()
{
  if (!rxEnabled) return;


  if (microsFromLastRX == 0)  // First pull down, initialization for message
    microsFromLastRX = micros();
  else
  {
    uint32_t deltaTime = microsFromLastRX - micros();
    if (deltaTime > 200) rxEnabled = 0; // Disable RX since one message has already been read
    
    uint8_t pulses = (deltaTime)/96;
    for (uint8_t i = 0; i<pulses; i++)
      rxBuffer = (rxBuffer << 1) | !digitalRead(INPUT_PIN);

    microsFromLastRX = micros();
    
  }
}
*/

void setup()
{
  Serial.begin(115200);


  pinMode(OUTPUT_PIN, OUTPUT);
  writePin(OUTPUT_PIN, 1);

  pinMode(INPUT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INPUT_PIN), incommingMessage, CHANGE);

  tmr0 = timerBegin(0, 80, true);
  timerAttachInterrupt(tmr0, &sendMessage, true);
  timerAlarmWrite(tmr0, 200*1000, true); // 1000 = 1ms
  timerAlarmEnable(tmr0);


  tmr1 = timerBegin(1, 80, true);
  timerAttachInterrupt(tmr1, &messageBit, true);
  timerAlarmWrite(tmr1, 95, true); // 1000 = 1ms
  timerAlarmDisable(tmr1);
  timerStop(tmr1);


}

void loop()
{
    Serial.println(rxBuffer, BIN);
    delay(600);
}