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

// DFRobot object
SoftwareSerial dfSerial(DFPLAYER_RX, DFPLAYER_TX);
DFRobotDFPlayerMini dfPlayer;

// Constants
const unsigned long NO_MOTION_TIMEOUT = 60 * 1000UL; // 30 seconds
const int FADE_DURATION = 1000; // 1 second fade
const int TOTAL_TRACKS = 35;

// Variables
unsigned long lastMotionTime = 0;
unsigned long fadeStartTime = 0;
int currentVolume = 15;
int lastVolume = currentVolume;
bool isPlaying = false;
bool isFadingOut = false;

void setup() {

  // Setup up the motion sensor and dfplayer pins
  pinMode(BUSY_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  // Start debugging serial
  Serial.begin(9600);
  dfSerial.begin(9600);

  // Seed random number generator for selecting random tracks
  randomSeed(analogRead(0)); 
  
  // Debounce
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
  // Serial.print("Motion: "); Serial.print(motionDetected);
  // Serial.print(" | Playing: "); Serial.print(isPlaying);
  // Serial.print(" | Busy: "); Serial.print(isPlayerBusy);
  // Serial.print(" | Volume: "); Serial.println(currentVolume);
  
  // Motion detection logic
  if (motionDetected) {
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
  
  delay(100);
}

void startPlayback() {
  //Serial.println("Starting playback");
  isPlaying = true;
  playRandomTrack();
}

// This one is important to do in code because if a track finishes playing before the chunk length
// it has to fade otherwise it will sound awkward. 
void startFadeOut() {
  //Serial.println("Starting fade out");
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
    //Serial.println("Playback stopped");
  } else {
    // Calculate and set intermediate volume
    float progress = (float)elapsed / FADE_DURATION;
    int fadeVolume = currentVolume * (1.0 - progress);
    dfPlayer.volume(fadeVolume);
  }
}

void playRandomTrack() {
  int trackNumber = random(1, TOTAL_TRACKS + 1);
  //Serial.print("Playing track: "); Serial.println(trackNumber);
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
