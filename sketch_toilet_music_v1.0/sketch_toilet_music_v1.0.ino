#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
// Pins
#define DFPLAYER_RX 10
#define DFPLAYER_TX 11 
#define BUSY_PIN 6
#define PIR_PIN 8
#define ROTARY_S1 5 
#define ROTARY_S2 4
#define KEY 3

SoftwareSerial dfSerial(DFPLAYER_RX, DFPLAYER_TX);
DFRobotDFPlayerMini dfPlayer;

// Constants
const unsigned long NO_MOTION_TIMEOUT =  10 * 1000UL; // 60 seconds
const int FADE_DURATION = 5000; // 5 second fade
const int TOTAL_TRACKS = 21;

// Variables
unsigned long lastMotionTime = 0;
unsigned long fadeStartTime = 0;
int currentVolume = 5;
int lastVolume = currentVolume;
bool isPlaying = false;
bool isFadingOut = false;
bool isFadingIn = false;
unsigned fadeTargetVolume = 0;

bool isProcessingPlayback = false;
const unsigned long MOTION_COOLDOWN = 2000; // 2 seconds cooldown
unsigned long lastMotionDetectionTime = 0;

void setup() {
  pinMode(BUSY_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(9600);
  dfSerial.begin(9600);

  randomSeed(analogRead(0)); // Seed random number generator
  
  delay(1000);
  Serial.println("Starting up.");

  if(!dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer initialization failed");
    while(true);
  }
  
  dfPlayer.volume(currentVolume);
}

void loop() {
  // Read inputs
  bool motionDetected = digitalRead(PIR_PIN) == HIGH;
  bool isPlayerBusy = digitalRead(BUSY_PIN) == LOW; // Typically LOW when playing
  int rotaryValue1 = digitalRead(ROTARY_S1);
  int rotaryValue2 = digitalRead(ROTARY_S2);
  bool keyValue = digitalRead(KEY);
  
  // Handle volume changes
  handleVolumeControl(rotaryValue1, rotaryValue2, keyValue);
  
  // Debug output
  Serial.print("Motion: "); Serial.print(motionDetected);
  Serial.print(" | Playing: "); Serial.print(isPlaying);
  Serial.print(" | Busy: "); Serial.println(isPlayerBusy);
  Serial.print(" | currentVolume: "); Serial.println()
  
  // Motion detection logic
  if (motionDetected && !isProcessingPlayback && 
    (millis() - lastMotionDetectionTime > MOTION_COOLDOWN)) {
  lastMotionDetectionTime = millis();
  lastMotionTime = millis();
  
  if (!isPlaying) {
    isProcessingPlayback = true;
    startPlayback();
    isProcessingPlayback = false;
  }
  
  // Cancel any ongoing fade-out
  if (isFadingOut) {
    currentVolume = dfPlayer.readVolume();
    fadeTargetVolume = lastVolume;
    isFadingOut = false;
    isFadingIn = true;
    fadeStartTime = millis();
    // isFadingOut = false;
    // dfPlayer.volume(currentVolume);
  }
}
  
  // Handle playback state
  if (isPlaying) {
    // Check if we need to stop due to timeout
    if (millis() - lastMotionTime > NO_MOTION_TIMEOUT && !isFadingOut && !isFadingIn) {
      startFadeOut();
    }
    
    // Play next track if current one finished
    if (!isPlayerBusy && !isFadingOut && !isFadingIn) {
      playRandomTrack();
    }
  }
  
  // Handle fade-out
  if (isFadingOut || isFadingIn) {
    handleFade();
  }
  
  delay(500);
}

void startPlayback() {
  Serial.println("Starting playback with fade");
  isPlaying = true;

  dfPlayer.volume(0);
  fadeTargetVolume = currentVolume;

  isFadingIn = true;
  fadeStartTime = millis();

  // currentVolume = lastVolume;
  // dfPlayer.volume(currentVolume);
  playRandomTrack();
}

void startFadeOut() {
  // Serial.println("Starting fade out");
  // isFadingOut = true;
  // fadeStartTime = millis();

  if(isFadingIn) {
    fadeTargetVolume = currentVolume;
    currentVolume = dfPlayer.readVolume();
    isFadingIn = false;
  }

  Serial.println("Start fade out");
  isFadingOut = true;
  fadeStartTime = millis();
}

void handleFade() {
  unsigned long elapsed = millis() - fadeStartTime;
  float progress = constrain((float)elapsed / FADE_DURATION, 0.0, 1.0);
  
  if(isFadingIn) {
    int currentFadeVolume = progress * fadeTargetVolume;
    dfPlayer.volume(currentFadeVolume);

    if(progress >= 1.0) {
      isFadingIn = false;
      dfPlayer.volume(fadeTargetVolume);
      Serial.println("Fade comlete");
    }
  }
  else if (isFadingOut) {
    int currentFadeVolume = (1.0 - progress) * currentVolume;
    dfPlayer.volume(currentFadeVolume);

    if(progress >= 1.0) {
      isFadingOut = false;
      isPlaying = false;
      dfPlayer.pause();
      Serial.println("Fade out complete playback stop");
    }
  }
  
  // if (elapsed >= FADE_DURATION) {
  //   // Fade complete
  //   isFadingOut = false;
  //   isPlaying = false;
  //   dfPlayer.pause();
  //   Serial.println("Playback stopped");
  // } else {
  //   // Calculate and set intermediate volume
  //   float progress = (float)elapsed / FADE_DURATION;
  //   int fadeVolume = currentVolume * (1.0 - progress);
  //   dfPlayer.volume(fadeVolume);
  // }
}

void playRandomTrack() {
  int trackNumber = random(1, TOTAL_TRACKS + 1);
  Serial.print("Playing track: "); Serial.println(trackNumber);
  dfPlayer.play(trackNumber);
}

void handleVolumeControl(int s1, int s2, int key) {
  // Volume down
  if (s2 == 0) {
    currentVolume = constrain(currentVolume - 3, 0, 30);
    dfPlayer.volume(currentVolume);
    delay(200); // Debounce
  }
  
  // Volume up
  if (s1 == 0) {
    currentVolume = constrain(currentVolume + 3, 0, 30);
    dfPlayer.volume(currentVolume);
    delay(200); // Debounce
  }
  
  // Mute toggle
  if (key == 0) {
    if (currentVolume == 0) {
      currentVolume = lastVolume;
    } else {
      lastVolume = currentVolume;
      currentVolume = 0;
    }
    dfPlayer.volume(currentVolume);
    delay(200); // Debounce
  }
}



























































//asd





