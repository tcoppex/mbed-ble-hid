///
///   ble_keyboard.ino
///   
///   created: 2020-11
///
///  Simulate an evil writing machine in an isolated hotel on winter. 
///  No hardware required other than an Arduino Nano 33 ble.
///

#include "Nano33BleHID.h"

#include "signal_utils.h"

/* -------------------------------------------------------------------------- */

static const char kSentence[] = "All work and no play makes Jack a dull boy";

static constexpr char kTickerChar = '.';
static constexpr int kNumTickerSteps = 4;

// How long it takes for the sentence to be written.
static constexpr uint32_t kSentenceDurationMilliseconds = 4029;

// How long it takes before the sentence is rewritten.
static constexpr uint32_t kSentenceDelayMilliseconds = 1977;

// How long one writing animation will run.
static constexpr uint32_t kSentenceTotalTimeMilliseconds = kSentenceDurationMilliseconds + kSentenceDelayMilliseconds;

// Safeguard to terminate this mess of an app before going crazy.
static constexpr uint32_t kTotalRuntime = 8 * kSentenceTotalTimeMilliseconds;

// Builtin LED animation delays when disconnect. 
static constexpr uint32_t kLedBeaconDelayMilliseconds = 1185;
static constexpr uint32_t kLedErrorDelayMilliseconds = kLedBeaconDelayMilliseconds / 10;

// Builtin LED intensity when connected.
static constexpr int kLedConnectedIntensity = 30;

/* -------------------------------------------------------------------------- */

// Basic Keyboard service.
Nano33BleKeyboard bleKb("Shining Keyboard");

// Tracking index for the end of the writing animation ticker.
int sTickerIndex = -1;

/* -------------------------------------------------------------------------- */

/** Utility struct to send a text through the Keyboard HID for a given time. */
struct SentenceWriter {
  std::string sentence;
  int current_index;
  uint32_t duration_ms;

  SentenceWriter(const char* str, uint32_t duration_ms)
    : sentence(str)
    , current_index(-1)
    , duration_ms(duration_ms)
  {}

  void write(HIDKeyboardService &kb, uint32_t frame_time)
  {
    // Calculate the absolute time in the animation (in [0.0f, 1.0f[)
    float dt = frame_time / static_cast<float>(duration_ms);
    
    // Smoothly interpolate to add "lifeness".
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
  Serial.begin(9600);

  // Initialize both BLE and the HID.
  bleKb.initialize();

  // Launch the event queue that will manage both BLE events and the loop. 
  // After this call the main thread will be halted.
  MbedBleHID_RunEventThread();
}

void loop()
{
  // When disconnected, we animate the builtin LED to indicate the device state.
  if (bleKb.disconnected()) {
    animateLED(LED_BUILTIN, (bleKb.has_error()) ? kLedErrorDelayMilliseconds 
                                                : kLedBeaconDelayMilliseconds);
    return;
  }

  // When connected, we slightly dim the builtin LED.
  analogWrite(LED_BUILTIN, kLedConnectedIntensity);

  // Stop when we reach a certain runtime.
  if (bleKb.connection_time() > kTotalRuntime) {
    return;
  }

  // Retrieve the HIDService to update.
  auto *kb = bleKb.hid();

  // Local time in the looping animation.
  const uint32_t frame_time = bleKb.connection_time() % kSentenceTotalTimeMilliseconds;

  // The animation is divided in two parts :
  if (frame_time < kSentenceDurationMilliseconds)
  {
    // Write the sentence using the StringWriter object.
    stringWriter.write(*kb, frame_time);
  }
  else
  {
    // Wait by writing dots at a different speed using the same logic as StringWriter.
    
    // Second-part delta time.
    float dt = (frame_time-kSentenceDurationMilliseconds) / float(kSentenceDelayMilliseconds);
    
    // "Slow-out" filtering.
    dt = 1.0f - (1.0f - dt) * pow(dt, dt);
    
    // Detect current index based on time.
    const int current_index = floor(dt * kNumTickerSteps);
    
    // Update keystroke when index changes.
    if (current_index != sTickerIndex) {
      kb->sendCharacter(kTickerChar);
      sTickerIndex = current_index;
    }
  }
}

/* -------------------------------------------------------------------------- */