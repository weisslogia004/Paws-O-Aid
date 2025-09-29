#include <Servo.h>                // Library for controlling servo motors
#include <Wire.h>                 // I2C communication library for LCD
#include <LiquidCrystal_I2C.h>   // Library to control I2C LCD displays

Servo servo;                     // Create servo object for controlling dispenser flap

// --- Pin assignments ---
const int trigPin = 2;           // Ultrasonic sensor trigger pin
const int echoPin = 3;           // Ultrasonic sensor echo pin
const int redLedPin = 4;         // Red LED indicates cooldown
const int greenLedPin = 5;       // Green LED indicates ready state
const int coinPin = 7;           // IR sensor pin for coin detection
const int servoPin = 9;          // Servo motor control pin
const int buzzerPin = 6;         // Buzzer pin for sound feedback

// --- Ultrasonic measurement variables ---
long duration;                   // Time for ultrasonic pulse to return
int distance;                     // Calculated distance in cm

// --- Cooldown parameters ---
unsigned long lastDispenseTime = 0;  // Tracks last dispense time
const unsigned long cooldown = 11000; // 11 seconds cooldown between dispenses
bool inCooldown = false;              // Flag indicating cooldown is active

// --- LCD setup ---
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD object: address 0x27, 16 columns, 2 rows

// --- Flags ---
bool coinInserted = false;            // True if a coin has been detected
bool handWaveDetected = false;        // True if a hand wave is detected
bool readyBeepPlayed = false;         // Ensures ready beep plays only once per cooldown

void setup() {
  servo.attach(servoPin);             // Attach servo to its pin
  pinMode(trigPin, OUTPUT);           // Trigger pin as output
  pinMode(echoPin, INPUT);            // Echo pin as input
  pinMode(redLedPin, OUTPUT);         // Red LED as output
  pinMode(greenLedPin, OUTPUT);       // Green LED as output
  pinMode(coinPin, INPUT_PULLUP);     // Use internal pull-up for faster, stable IR detection
  pinMode(buzzerPin, OUTPUT);         // Buzzer as output

  servo.write(0);                     // Start servo at 0° (closed position)
  digitalWrite(redLedPin, LOW);       // Red LED OFF
  digitalWrite(greenLedPin, HIGH);    // Green LED ON (ready)
  digitalWrite(buzzerPin, LOW);       // Buzzer OFF

  lcd.init();                         // Initialize LCD
  lcd.backlight();                    // Turn on LCD backlight
  lcd.setCursor(0, 0);                // Move cursor to first line, first column
  lcd.print("Paws-o-Aid!");           // Display project name
  lcd.setCursor(0, 1);                // Move cursor to second line
  lcd.print("Insert Coin...  ");      // Prompt user to insert a coin

  // --- Initial ready beep ---
  digitalWrite(buzzerPin, HIGH);      // Turn buzzer ON
  delay(100);                         // Beep duration 100ms
  digitalWrite(buzzerPin, LOW);       // Turn buzzer OFF
  readyBeepPlayed = true;             // Flag that ready beep has played
}

void loop() {
  unsigned long currentTime = millis(); // Track current time

  // --- Ultrasonic measurement ---
  digitalWrite(trigPin, LOW);          // Ensure trigger starts LOW
  delayMicroseconds(2);                // Short delay for stability
  digitalWrite(trigPin, HIGH);         // Send 10µs HIGH pulse
  delayMicroseconds(10);               // Pulse duration
  digitalWrite(trigPin, LOW);          // End pulse
  duration = pulseIn(echoPin, HIGH, 20000); // Measure echo duration (timeout 20ms)
  distance = duration * 0.034 / 2;          // Convert pulse duration to distance in cm

  // --- Cooldown handling ---
  if (inCooldown) {
    long elapsed = currentTime - lastDispenseTime; // Time since last dispense
    long remaining = (cooldown - elapsed) / 1000;  // Remaining cooldown in seconds
    if (remaining < 0) remaining = 0;             // Prevent negative display

    digitalWrite(redLedPin, HIGH);                // Red LED ON during cooldown
    digitalWrite(greenLedPin, LOW);              // Green LED OFF

    lcd.setCursor(0, 1);                          // LCD second line
    lcd.print("Wait for: ");
    lcd.print(remaining);                         // Show remaining cooldown
    lcd.print("s     ");                          // Clear leftover characters

    readyBeepPlayed = false;                       // Reset ready beep flag for next cycle

    if (elapsed >= cooldown) {                     // Cooldown finished
      inCooldown = false;                          // Exit cooldown
      coinInserted = false;                        // Reset coin flag
      handWaveDetected = false;                    // Reset hand wave flag
      digitalWrite(redLedPin, LOW);               // Red LED OFF
      digitalWrite(greenLedPin, HIGH);            // Green LED ON

      lcd.setCursor(0, 1);
      lcd.print("Insert Coin...  ");               // Prompt again

      // --- Ready beep after cooldown ---
      if (!readyBeepPlayed) {                      // Only play once
        digitalWrite(buzzerPin, HIGH);           
        delay(150);                                // Beep duration
        digitalWrite(buzzerPin, LOW);
        readyBeepPlayed = true;                    // Set flag
      }
    }
  }

  // --- Instant Coin detection ---
  if (!inCooldown && !coinInserted && digitalRead(coinPin) == LOW) { // LOW = coin detected
    coinInserted = true;                        // Set coin flag
    lcd.setCursor(0, 1);
    lcd.print("Coin OK! Wave  ");              // Prompt for hand wave

    // --- Coin beep ---
    digitalWrite(buzzerPin, HIGH);           
    delay(50);   // Shorter beep for faster feel
    digitalWrite(buzzerPin, LOW);
  }

  // --- Hand wave detection ---
  if (coinInserted && distance > 0 && distance < 10) { // Hand detected within 10 cm
    handWaveDetected = true;                           // Set hand wave flag
  }

  // --- Dispense if both coin & wave ---
  if (coinInserted && handWaveDetected && !inCooldown) {
    lcd.setCursor(0, 1);
    lcd.print("Dispensing...  ");

    // --- Servo operation ---
    servo.write(90);                           // Open dispenser (90°)
    delay(700);                                // Hold for 700ms to release treat
    servo.write(0);                             // Close dispenser

    // --- Double beep during dispensing ---
    for (int i = 0; i < 2; i++) {
      digitalWrite(buzzerPin, HIGH);
      delay(150);                               // Beep duration
      digitalWrite(buzzerPin, LOW);
      delay(100);                               // Gap between beeps
    }

    // --- Thank You message ---
    lcd.setCursor(0, 1);
    lcd.print("Thank You!     ");               // Display for 3 seconds
    delay(3000);

    lastDispenseTime = millis();               // Start cooldown
    inCooldown = true;                          // Set cooldown flag
    readyBeepPlayed = false;                    // Ready beep will play after cooldown
  }

  delay(5); // Very short delay for almost instantaneous IR response
} 