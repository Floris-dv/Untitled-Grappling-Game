#pragma once

struct TestLoggingOptions {
  uint8_t failures;         // until which depth failures should be logged
  uint8_t verbosefailures;  // until which depth failures should be logged
                            // verbosely
  uint8_t successes;        // until which depth everything should be logged
  uint8_t verbosesuccesses; // until which depth everything should be logged
                            // verbosely
};

// continueIfFailed: if true, continue the test even though it failed
bool TestAll(TestLoggingOptions testLoggingOptions,
             bool continueIfFailed = true);
bool TestPhysics(TestLoggingOptions testLoggingOptions, unsigned int depth,
                 bool continueIfFailed = true);
bool TestWindow(TestLoggingOptions testLoggingoptions, unsigned int depth,
                bool continueifFailed = true);
bool TestAudioSystem(TestLoggingOptions testLoggingoptions, unsigned int depth,
                     bool continueifFailed = true);
