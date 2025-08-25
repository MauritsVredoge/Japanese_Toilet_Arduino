#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <EEPROM.h>

// Pins
#define DFPLAYER_RX 10
#define DFPLAYER_TX 11 
#define BUSY_PIN 6
#define PIR_PIN 8
#define ROTARY_S1 5 
#define ROTARY_S2 4
#define KEY 3

// EEPROM addresses
#define VOLUME_ADDR 0
#define LAST_VOLUME_ADDR 1
#define LAST_SAVE_TIME_ADDR 2 // Address for storing last save time in memory

// Constants
const unsigned long NO_MOTION_TIMEOUT = 60 * 1000UL; // 60 seconds long no motion
const int FADE_DURATION = 1000; // 1 second fade
const int TOTAL_TRACKS = 35;
const unsigned long SAVE_INTERVAL = 6 * 60 * 60 * 1000UL; // 6 hours
const unsigned long ROTARY_DEBOUNCE_DELAY = 50; // 50ms debounce for rotary encoder

// DFRobot object
SoftwareSerial dfSerial(DFPLAYER_RX, DFPLAYER_TX);
DFRobotDFPlayerMini dfPlayer;

// Variables
unsigned long lastMotionTime = 0;
unsigned long fadeStartTime = 0;
unsigned long lastSaveTime = 0;
unsigned long lastRotaryTime = 0; // For rotary encoder debouncing
int currentVolume = 15;
int lastVolume = currentVolume;
bool isPlaying = false;
bool isFadingOut = false;
bool volumeChanged = false;

void setup() {
  // Setup up the motion sensor and dfplayer pins
  pinMode(BUSY_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(ROTARY_S1, INPUT_PULLUP);
  pinMode(ROTARY_S2, INPUT_PULLUP);
  pinMode(KEY, INPUT_PULLUP);

  // Start debugging serial
  Serial.begin(9600);
  dfSerial.begin(9600);

  // Load saved volume from EEPROM
  loadVolumeSettings();
  loadLastSaveTime();

  // Seed random number generator for selecting random tracks
  randomSeed(analogRead(0)); 
  
  // Debounce
  delay(1000);
  // Serial.println("Starting up.");
  // Serial.print("Loaded volume: "); Serial.println(currentVolume);
  // Serial.print("Loaded last volume: "); Serial.println(lastVolume);
  // Serial.print("Last save time: "); Serial.println(lastSaveTime);

  if(!dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer initialization failed");
    while(true);
  }
  
  dfPlayer.volume(currentVolume);
}

void loop() {
  // Read inputs
  bool motionDetected = digitalRead(PIR_PIN) == HIGH;
  bool isPlayerBusy = digitalRead(BUSY_PIN) == LOW;
  
  // Handle volume changes with debouncing
  handleVolumeControl();
  
  // Check if we should save to EEPROM (every 6 hours or on significant events)
  checkAndSaveVolume();
  
  // Motion detection logic
  if (motionDetected && currentVolume > 0) {
    lastMotionTime = millis();
    
    if (!isPlaying) {
      startPlayback();
    }
    
    // Cancel any ongoing fade-out
    if (isFadingOut) {
      isFadingOut = false;
      dfPlayer.volume(currentVolume);
    }
  }
  
  // Handle playback state
  if (isPlaying) {
    // Check if we need to stop due to timeout
    if (millis() - lastMotionTime > NO_MOTION_TIMEOUT && !isFadingOut) {
      startFadeOut();
    }
    
    // Play next track if current one finished
    if (!isPlayerBusy && !isFadingOut) {
      playRandomTrack();
    }
  }
  
  // Handle fade-out
  if (isFadingOut) {
    handleFade();
  }
  
  delay(10); // Shorter delay for more responsive debouncing
}

void loadVolumeSettings() {
  int savedVolume = EEPROM.read(VOLUME_ADDR);
  int savedLastVolume = EEPROM.read(LAST_VOLUME_ADDR);
  
  if (savedVolume >= 0 && savedVolume <= 30) {
    currentVolume = savedVolume;
  }
  
  if (savedLastVolume >= 0 && savedLastVolume <= 30) {
    lastVolume = savedLastVolume;
  }
}

void loadLastSaveTime() {
  // Read 4 bytes for the unsigned long
  lastSaveTime = 0;
  for (int i = 0; i < 4; i++) {
    byte part = EEPROM.read(LAST_SAVE_TIME_ADDR + i);
    lastSaveTime = (lastSaveTime << 8) | part;
  }
}

void saveVolumeSettings() {
  EEPROM.write(VOLUME_ADDR, currentVolume);
  EEPROM.write(LAST_VOLUME_ADDR, lastVolume);
  
  unsigned long currentTime = millis();
  for (int i = 0; i < 4; i++) {
    EEPROM.write(LAST_SAVE_TIME_ADDR + i, (currentTime >> (8 * (3 - i))) & 0xFF);
  }
  lastSaveTime = currentTime;
  
  volumeChanged = false;
  Serial.print("Volume saved to EEPROM: "); Serial.println(currentVolume);
}

void checkAndSaveVolume() {
  if (volumeChanged) {
    unsigned long currentTime = millis();
    
    // Handle millis() overflow because at some point the timer will exceed max int
    // ongeveer elke 49 dagen.
    if (currentTime < lastSaveTime) {
      // Overflow occurred, force save
      saveVolumeSettings();
      Serial.println("Millis overflow detected, forced save");
    } else if (currentTime - lastSaveTime >= SAVE_INTERVAL) {
      // 6 hours have passed, save to EEPROM
      saveVolumeSettings();
    }
  }
}

void startPlayback() {
  isPlaying = true;
  playRandomTrack();
}

void startFadeOut() {
  isFadingOut = true;
  fadeStartTime = millis();
}

void handleFade() {
  unsigned long elapsed = millis() - fadeStartTime;
  
  if (elapsed >= FADE_DURATION) {
    // Fade complete
    isFadingOut = false;
    isPlaying = false;
    dfPlayer.pause();
    // Reset the volume after fading to 0 and pausing.
    dfPlayer.volume(currentVolume);
    Serial.println("Fade complete. Volume reset.");
  } else {
    // Calculate and set intermediate volume
    float progress = (float)elapsed / FADE_DURATION;
    int fadeVolume = currentVolume * (1.0 - progress);
    dfPlayer.volume(fadeVolume);
  }
}

void playRandomTrack() {
  int trackNumber = random(1, TOTAL_TRACKS + 1);
  dfPlayer.play(trackNumber);
}

void handleVolumeControl() {
  unsigned long currentTime = millis();
  
  // Debounce check
  if (currentTime - lastRotaryTime < ROTARY_DEBOUNCE_DELAY) {
    return;
  }
  
  int rotaryValue1 = digitalRead(ROTARY_S1);
  int rotaryValue2 = digitalRead(ROTARY_S2);
  bool keyValue = digitalRead(KEY) == LOW;
  
  bool volumeAdjusted = false;
  
  // Handle rotary rotation ONLY if not muted
  if (currentVolume > 0) {
    if (rotaryValue2 == LOW) {
      currentVolume = constrain(currentVolume - 1, 1, 30);
      volumeAdjusted = true;
    }
    if (rotaryValue1 == LOW) {
      currentVolume = constrain(currentVolume + 1, 1, 30);
      volumeAdjusted = true;
    }
  }
  
  // Handle button press Mute Toggle
  if (keyValue) {
    if (currentVolume == 0) {
      // Unmute: restore last volume
      currentVolume = lastVolume;
    } else {
      // Mute: remember current volume and set to 0
      lastVolume = currentVolume;
      currentVolume = 0;
    }
    volumeAdjusted = true;
  }
  
  if (volumeAdjusted) {
    dfPlayer.volume(currentVolume);
    volumeChanged = true;
    lastRotaryTime = currentTime;
    Serial.print("Volume changed: "); Serial.println(currentVolume);
    delay(20);
  }
}