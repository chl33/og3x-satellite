// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "og3/base-station.h"
#include "unity.h"

void setUp() {
  // set stuff up here
}

void tearDown() {
  // clean stuff up here
}

void test_packet() {}

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test_packet);
  return UNITY_END();
}

int main(int argc, char** argv) { runUnityTests(); }

// For arduion framework
void setup() {}
void loop() {}

// For ESP-IDF framework
void app_main() { runUnityTests(); }
