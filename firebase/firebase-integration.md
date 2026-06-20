# Firebase Integration

Firebase Realtime Database (RTDB) serves as the communication layer connecting the sensor system, mobile application, and embedded controller.

## Data Flow

### Sensor → Cloud → Mobile Application

- The NPK sensor collects soil nutrient readings from the environment.
- The embedded system processes the sensor data and uploads the readings to Firebase RTDB.
- The mobile application retrieves the stored data from Firebase and displays real-time soil nutrient information for monitoring and visualization.

### Mobile Application → Cloud → Microcontroller → Actuators

- User commands from the mobile application are sent and stored in Firebase RTDB.
- The microcontroller retrieves these commands from Firebase.
- The embedded system executes the required actions by controlling actuators, including fertilizer valves and the mixing motor.

## Purpose

This architecture enables real-time monitoring and remote control by integrating sensor data storage, mobile application visualization, and automated hardware operation through Firebase synchronization.
