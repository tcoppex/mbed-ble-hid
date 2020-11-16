///
///   ble_shining_kb.ino
///   
///   created: 2020-11
///
///  Simulate an evil writing machine in an isolated hotel on winter. 
///  No hardware required other than an Nano33 ble.
///

#include "Nano33BleHID.h"
#include "signal_utils.h"

/* -------------------------------------------------------------------------- */

Nano33BleKeyboard bleKb("Shining Keyboard");

// Tracking index for the end of the writing animation ticker.
int sTickerIndex = -1;

static const char kSentence[] = "All work and no play makes Jack a dull boy";
static const int kNumTickerSteps = 4;

// How long it takes for the sentence to be written.
static const int kSentenceDurationMilliseconds = 4029;

// How long it takes before the sentence is rewritten.
static const int kSentenceDelayMilliseconds = 1977;

// How long one writing animation will run.
static const int kSentenceTotalTimeMilliseconds = kSentenceDurationMilliseconds + kSentenceDelayMilliseconds;

// Safeguard to terminate this mess of an app before going crazy.
static const int kTotalRuntime = 8 * kSentenceTotalTimeMilliseconds;

// Builtin LED animation delays when disconnect. 
static const int kLedBeaconDelayMilliseconds = 1185;
static const int kLedErrorDelayMilliseconds = kLedBeaconDelayMilliseconds / 10;

// Builtin LED intensity when connected.
static const int kLedConnectedIntensity = 30;

/* -------------------------------------------------------------------------- */

/** Utility struct to send a text through the Keyboard HID for a given time. */
struct SentenceWriter {
  std::string sentence;
  int current_index;
  unsigned long duration_ms;

  SentenceWriter(const char* str, unsigned long duration_ms)
    : sentence(str)
    , current_index(-1)
    , duration_ms(duration_ms)
  {}

  void write(HIDKeyboardService &kb, unsigned long frame_time)
  {
    // Calculate the absolute time in the animation (in [0.0f, 1.0f[)
    float dt = frame_time / float(duration_ms);
    
    // Add a smooth interpolation to add "lifeness".
    dt = smoothstep(0.0f, 1.0f, dt);
    
    // Map absolute time to the letter index.
    const int index = floor(dt * sentence.size());

    // When the index change, send its letter via the keyboard service.
    if (current_index != index) {
      current_index = index;
      const uint8_t letter = sentence[index];
      kb.sendCharacter(letter);
    }
  } 
} stringWriter(kSentence, kSentenceDurationMilliseconds);

/* -------------------------------------------------------------------------- */

void setup()
{
  // General setup.
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize both BLE and the HID.
  bleKb.initialize();

  // Launch the event queue that will manage both BLE events and the loop. 
  // After this call the main thread will be halted.
  MbedBleHID_RunEventThread();
}

void loop()
{
  // When disconnected, we animate the builtin LED to indicate the device state.
  if (bleKb.connected() == false) {
    animateLED(LED_BUILTIN, (bleKb.has_error()) ? kLedErrorDelayMilliseconds 
                                                : kLedBeaconDelayMilliseconds);
    return;
  }

  // When connected, we slightly dim the builtin LED.
  analogWrite(LED_BUILTIN, kLedConnectedIntensity);

  // Stop when we reach a certain runtime (better safe than sorry).
  if (bleKb.connection_time() > kTotalRuntime) {
    return;
  }

  // Retrieve the HIDService to update.
  auto *kb = bleKb.hid();

  // Local time in the looping animation.
  unsigned long frame_time = bleKb.connection_time() % kSentenceTotalTimeMilliseconds;

  // The animation is divided in two parts :
  if (frame_time < kSentenceDurationMilliseconds)
  {
    // Write the sentence using the StringWriter object.
    stringWriter.write(*kb, frame_time);
  }
  else
  {
    // Wait by writing dots at a different speed (using the same logic as a StringWriter).
    float dt = (frame_time-kSentenceDurationMilliseconds) / float(kSentenceDelayMilliseconds);
          dt = 1.0f - (1.0f - dt) * pow(dt, dt); // slow-out
    int index = floor(kNumTickerSteps*dt);
    if (sTickerIndex != index) {
      kb->sendCharacter('.');
      sTickerIndex = index;
    }
  }
}

/* -------------------------------------------------------------------------- */
