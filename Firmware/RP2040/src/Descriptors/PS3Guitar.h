#pragma once

#include <stdint.h>

namespace PS3Guitar {
// PS3 Guitar Hero Controller Report
// Based on community reverse engineering and comparison with DualShock 3
struct InReport {
  uint8_t report_id; // Byte 0: Often 0x01

  // Byte 1: Buttons and D-Pad (Standard PS3 mapping)
  // Bit 0: Select
  // Bit 1: L3
  // Bit 2: R3
  // Bit 3: Start
  // Bit 4: D-Pad Up (Strum Up)
  // Bit 5: D-Pad Right
  // Bit 6: D-Pad Down (Strum Down)
  // Bit 7: D-Pad Left
  uint8_t buttons0;

  // Byte 2: More Buttons
  // Bit 0: L2
  // Bit 1: R2
  // Bit 2: L1 (Orange Fret)
  // Bit 3: R1
  // Bit 4: Triangle (Yellow Fret)
  // Bit 5: Circle (Red Fret)
  // Bit 6: Cross (Blue Fret - Note: swapped vs standard controller color)
  // Bit 7: Square (Green Fret)
  uint8_t buttons1;

  // Byte 3: PS Button / Reserved
  uint8_t buttons2;

  // Byte 4: Reserved
  uint8_t reserved0;

  // Byte 5: Joystick LX (Analog Stick X)
  uint8_t joystick_lx;

  // Byte 6: Joystick LY (Analog Stick Y)
  uint8_t joystick_ly;

  // Byte 7: Joystick RX (Whammy Bar usually here or Z-axis)
  uint8_t joystick_rx;

  // Byte 8: Joystick RY
  uint8_t joystick_ry;

  // Bytes 9-40: Pressure sensitive buttons and other data
  uint8_t reserved1[32];

  // Bytes 41-50: Motion Sensors (Big Endian)
  // 41-42: Accel X (Tilt)
  // 43-44: Accel Y
  // 45-46: Accel Z
  // 47-48: Gyro Z
  uint16_t accel_x; // Bytes 41-42 (Index 41 in 0-based array)
  uint16_t accel_y;
  uint16_t accel_z;
  uint16_t gyro_z;
} __attribute__((packed));

struct Buttons0 {
  static const uint8_t SELECT = 0x01;
  static const uint8_t L3 = 0x02;
  static const uint8_t R3 = 0x04;
  static const uint8_t START = 0x08;
  static const uint8_t STRUM_UP = 0x10; // D-Pad Up
  static const uint8_t DPAD_RIGHT = 0x20;
  static const uint8_t STRUM_DOWN = 0x40; // D-Pad Down
  static const uint8_t DPAD_LEFT = 0x80;
};

struct Buttons1 {
  static const uint8_t L2 = 0x01;
  static const uint8_t R2 = 0x02;
  static const uint8_t ORANGE = 0x04; // L1
  static const uint8_t R1 = 0x08;
  static const uint8_t YELLOW = 0x10; // Triangle
  static const uint8_t RED = 0x20;    // Circle
  static const uint8_t BLUE = 0x40;   // Cross
  static const uint8_t GREEN = 0x80;  // Square
};

struct Buttons2 {
  static const uint8_t PS = 0x01; // Bit 0 of Byte 3
};
} // namespace PS3Guitar
