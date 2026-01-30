# Remote Controlled Car Project

<table>
  <tr>
    <td><img src="PhotosAndVideos/TopView.jpeg" alt="RC Car Top View" width="500"></td>
    <td><img src="PhotosAndVideos/FrontView.jpeg" alt="RC Car Full View" width="500"></td>
  </tr>
</table>


Fully custom remote controlled car. Specced and sourced all componenets, ensuring mechanical and electrical compatibaility. Designed car assembly and modeled all components. 3D printed chassis, steering mechanism, and component mounts. Assembled the car and safely integrated the electronics. Programmed car and remote control with ESP-NOW communication and IMU-based motion control. 

---

### 1. Mechanical
Created a full CAD design of the car assembly. STEP file of the full assembly is available [HERE](CAD/RCCarAssembly.STEP).
<img src="CAD/Images/FullAssembly.png" alt="FullAssembly" width="500">

Designed and 3D printed mounts for the battery, driver motor, and ESP32, as well as the steering mechanism linkages and the chassis
<table>
  <tr>
    <td><img src="CAD/Images/BatteryMount.png" alt="BatteryMount" width="500"></td>
    <td><img src="CAD/Images/MotorMount.png" alt="MotorMount" width="500"></td>
  </tr>
</table>
<table>
  <tr>
    <td><img src="CAD/Images/SteeringAssembly.png" alt=SteeringMechanism" width="500"></td>
    <td><img src="CAD/Images/Chassis.png" alt="Chassis" width="500"></td>
  </tr>
</table>


---

### 2. Electrical
Specced and sourced all componenets, ensuring mechanical and electrical compatibaility.
Electronics were integrated with secure connections using a combination of screw terminals, soldered joints, and JST connectors.

High Level Circuit Diagram for the car and controller:
<img src="CircuitDiagrams.jpg" alt="Circuit Diagrams" width="850">


---

### 3. Software
Car and remote programmed on ESP32 microcontrollers using Arduino IDE.

- Motion-based steering and throttle using an MPU6050 Inertial Measurement Unit  
- Angle calculated using a **complementary filter** combining accelerometer and gyroscope data  
- Steering limited to ±60° for safety, with a dead zone to prevent accidental movement  
- Motion-enabled driving only engages when a dedicated controller button is pressed  
- Communication between car and remote handled via ESP-NOW at a 10 Hz update rate
- PWM control for motor speed and direction

Code folder contains source files for car and remote
---



