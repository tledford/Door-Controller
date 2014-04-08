#define MAX_BITS 100                 // max number of bits
#define WEIGAND_WAIT_TIME  3000      // time to wait for another weigand pulse.

#include <SPI.h>
#include <Servo.h>

Servo myservo;  // create servo object to control a servo

int lock = 0;    //servo unlock position
int unlock = 160;    //servo lock position
int old = 0;    //servo intial and old position
int ledPin = 13;  //led is on when door is locked
int greenPin = 6;
int switchPin = 7; //momentary connected to ground for lock toggling.
int switchRead = 1;

unsigned char databits[MAX_BITS];    // stores all of the data bits
unsigned char bitCount;              // number of bits currently captured
unsigned char flagDone;              // goes low when data is currently being captured
unsigned int weigand_counter;        // countdown until we assume there are no more bits

unsigned long facilityCode=0;        // decoded facility code
unsigned long cardCode=0;

const int numCards = 5;
unsigned long authorizedCards[numCards] = {4597, 19328, 14526, 13519, 16594};
//tommy, tommySummerArts, jeremy, zach, steve

// interrupt that happens when INTO goes low (0 bit)
void ISR_INT0()
{
    //Serial.print("0");
    bitCount++;
    flagDone = 0;
    weigand_counter = WEIGAND_WAIT_TIME;
}

// interrupt that happens when INT1 goes low (1 bit)
void ISR_INT1()
{
    //Serial.print("1");
    databits[bitCount] = 1;
    bitCount++;
    flagDone = 0;
    weigand_counter = WEIGAND_WAIT_TIME;
}

void setup()
{
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    myservo.attach(5);  //the pin for the servo control
    lockServo(); //initialize the servo to the locked position incase of random reset
    pinMode(2, INPUT);     // DATA0 (INT0)
    pinMode(3, INPUT);     // DATA1 (INT1)
    
    
    // binds the ISR functions to the falling edge of INTO and INT1
    attachInterrupt(0, ISR_INT0, FALLING);
    attachInterrupt(1, ISR_INT1, FALLING);
    
    weigand_counter = WEIGAND_WAIT_TIME;
    
    //momentary switch initializing
    pinMode(switchPin, INPUT_PULLUP);
}

void loop()
{
  
    while (Serial.available() == 0)
    {
      switchRead = digitalRead(switchPin);
      if (!switchRead) //if pushbutton is pressed
      {
        delay(500);
          if(switchRead == digitalRead(switchPin))
          {
            if(old != lock)
            {
              lockServo();
            }
            else if(old != unlock)
            {
              unlockServo();
            }
          }
          //switchRead = 1;
      }
      
        
        // This waits to make sure that there have been no more data pulses before processing data
        if (!flagDone)
        {
            if (--weigand_counter == 0)
                flagDone = 1;
        }
        
        // if we have bits and we the weigand counter went out
        if (bitCount > 0 && flagDone)
        {
            unsigned char i;
            
            if (bitCount == 35)
            {
                // 35 bit HID Corporate 1000 format
                for (i=2; i<14; i++)
                {
                    //Serial.print(databits[i]);
                    facilityCode <<=1;
                    //Serial.println(facilityCode);
                    facilityCode |= databits[i];
                    //Serial.println(facilityCode);
                    //Serial.println("");
                }
                for (i=14; i<34; i++)
                {
                    //Serial.print(databits[i]);
                    cardCode <<=1;
                    //Serial.println(facilityCode);
                    cardCode |= databits[i];
                    //Serial.println(facilityCode);
                    //Serial.println("");
                }
                /*Serial.print("Facility Code: ");
                Serial.println(facilityCode);
                Serial.println("");
                Serial.print("Card Code: ");
                Serial.println(cardCode);
                Serial.println("");
                */
                for(int i = 0; i < numCards; i++)
                {
                    if (cardCode == authorizedCards[i])
                    {
                        if(old != lock)
                        {
                            lockServo();
                        }
                        else if(old != unlock)
                        {
                            unlockServo();
                        }
                        break;
                    }
                }
            }
            
            // cleanup and get ready for the next card
            bitCount = 0;
            facilityCode = 0;
            cardCode = 0;
            for (i=0; i<MAX_BITS; i++)
            {
                databits[i] = 0;
            }
        }
    }
    
    int val = Serial.read() - '0';
    
    if(val == 1 && old != lock)
    {
        lockServo();
    }
    else if(val == 2 && old != unlock)
    {
        unlockServo();
    }
}

void lockServo()
{
    myservo.attach(5);
    delay(20);
    digitalWrite(ledPin, HIGH);   // sets the LED on
                                  //Serial.println("LOCKED");
    
    //for(int j = old; j <= lock; j += 1)
    {
        myservo.write(lock);
        delay(1000);
    }
    myservo.detach();
    digitalWrite(greenPin, HIGH);
    //Serial.println("Locked");
    old = lock;
    Serial.write("1");
}

void unlockServo()
{
    myservo.attach(5);
    delay(20);
    digitalWrite(ledPin, LOW);
    //Serial.println("UNLOCKED");
    //for(int j = old; j>=unlock; j-=1)
    {
        myservo.write(unlock);
        delay(1000);
    }
    myservo.detach();
    digitalWrite(greenPin, LOW);
    //Serial.println("Unlocked");
    old = unlock;
    Serial.write("2");
}
