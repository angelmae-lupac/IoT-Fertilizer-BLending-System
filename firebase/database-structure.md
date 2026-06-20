# Firebase Realtime Database Structure

## Overview

Firebase Realtime Database (RTDB) was used as a cloud-based data storage and communication platform for the IoT-based fertilizer blending system. It stores sensor readings collected from the embedded system, allowing the mobile application to retrieve and visualize real-time soil NPK data.

The database also handles control commands from the mobile application. When users interact with the app, commands are written to Firebase RTDB, which are then read by the microcontroller to control the fertilizer valves and system operation.

## Data Structure

```json
{
  "NPK": {
    "Nitrogen": 26,
    "Phosphorus": 37,
    "Potassium": 74,
    "status": "OK"
  },
  "fertilized": {
    "valveK": 0,
    "valveN": 0,
    "valveP": 0
  },
  "mix": {
    "command": 0
  },
  "valve": {
    "K": 1,
    "N": 1,
    "P": 1,
    "command": "1"
  }
}
