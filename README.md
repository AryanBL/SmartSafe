# SmartSafe

SmartSafe is a dual-controller embedded safe/security system built around two **ATmega32** microcontrollers running at **8 MHz**. The project separates user-facing safe control from remote alarm/monitoring functions:

- **Master unit** — password entry, keypad handling, servo lock control, tamper sensing, power-failure detection, event logging, sleep/wake behavior, and communication with the Slave.
- **Slave unit** — remote LCD status display, green/red status LEDs, buzzer alarms, and processing of event messages received from the Master over USART.

The repository also includes a **Proteus simulation project**, Atmel Studio projects, GNU Makefiles, and engineering documentation for the main hardware/software modules.

## Features

### Master

- 4-digit password authentication
- 4×4 matrix keypad input
- 16×2 LCD password-entry interface
- Servo-controlled safe lock
- Failed-login counting and timed lockout
- Tamper detection through ADC
- Power-failure detection through the ATmega32 analog comparator
- Power-failure standby and restoration handling
- Inactivity sleep with external-button wake-up
- Watchdog reset detection
- Internal EEPROM storage for password/failure state
- External **25LC040 SPI EEPROM** event logging
- Log dump and clear commands over USART
- Event reporting to the Slave over USART

### Slave

- Remote 16×2 LCD status display
- Green success indicator
- Red alarm indicator
- Buzzer output
- Temporary failure/tamper alarms
- Longer security-breach alarm
- Persistent power-failure alarm until restoration
- Ignores log-dump traffic so log records do not accidentally trigger alarms

## System Architecture

```text
          +-----------------------+
          |    SmartSafe Master   |
          |      ATmega32         |
          |                       |
 Keypad ->| Password handling     |
 Tamper ->| ADC monitoring        |
 Power  ->| Analog comparator     |
          | Event logger          |<----> 25LC040 EEPROM
          | Servo control         |-----> Safe lock
          | LCD interface         |
          +-----------+-----------+
                      |
                      | USART, 9600 baud
                      v
          +-----------------------+
          |    SmartSafe Slave    |
          |      ATmega32         |
          |                       |
          | Remote LCD            |
          | Green / Red LEDs      |
          | Buzzer                |
          +-----------------------+
```

## Hardware Configuration

Both firmware projects target:

- **MCU:** ATmega32
- **Clock:** 8 MHz
- **Compiler:** AVR-GCC
- **Language:** C (GNU99 in the supplied Makefiles)
- **USART:** 9600 baud

### Master Pin Map

| Function | ATmega32 Pin(s) | Notes |
|---|---|---|
| LCD data | PC0–PC7 | 16×2 LCD, 8-bit mode |
| LCD RS | PA0 | LCD control |
| LCD EN | PA2 | LCD control |
| LCD R/W | GND | Tied low in hardware |
| Servo | PD5 / OC1A | Timer1 PWM output |
| Wake button | PD2 / INT0 | Active-low wake input |
| Keypad rows | PA4–PA7 | Outputs |
| Keypad columns | PD3, PD4, PB0, PB1 | Inputs with pull-ups |
| Tamper input | PA1 / ADC1 | Analog tamper sensor/potentiometer |
| Power monitor | PB3 / AIN1 | Analog-comparator divider input |
| EEPROM CS / SS | PB4 | 25LC040 chip select |
| EEPROM MOSI | PB5 | Hardware SPI |
| EEPROM MISO | PB6 | Hardware SPI |
| EEPROM SCK | PB7 | Hardware SPI |

### Slave Pin Map

| Function | ATmega32 Pin(s) | Notes |
|---|---|---|
| LCD data | PC0–PC7 | 16×2 LCD, 8-bit mode |
| LCD RS | PA0 | LCD control |
| LCD EN | PA2 | LCD control |
| LCD R/W | GND | Tied low in hardware |
| Buzzer | PA3 | Remote audible alarm |
| Green LED | PD6 | Success/status indicator |
| Red LED | PD7 | Alarm indicator |

## Master–Slave Protocol

The Master sends newline-terminated event messages to the Slave over USART. The Slave performs exact string comparisons before changing its outputs.

| Message | Slave behavior |
|---|---|
| `LOGIN_OK` | Shows access granted and turns on green LED temporarily |
| `FAILURE` | Shows wrong-password warning and activates temporary alarm |
| `BREACH` | Shows security breach and activates the longer alarm |
| `LOCKOUT_END` | Clears the temporary alarm and returns to waiting state |
| `Tamper` | Shows tamper warning and activates temporary alarm |
| `POWER_FAIL` | Shows power failure and keeps red LED + buzzer active |
| `POWER RESTORED` | Clears the persistent power alarm |
| `WDT_RESET` | Reports that the Master restarted after watchdog reset |

Log-dump markers and log records are deliberately ignored by the Slave.

## Security / Timing Behavior

Values currently defined in the Master configuration include:

- Maximum failed attempts: **3**
- Lockout duration: **30 seconds**
- Inactivity timeout before sleep: **10 seconds**
- Safe unlock duration: **5 seconds**
- Tamper UI hold: **5 seconds**
- Failed-password UI hold: **1 second**

The Slave currently uses:

- Standard remote alarm duration: **5 seconds**
- Security-breach alarm duration: **30 seconds**
- Success LED duration: **5 seconds**

These values can be changed in the respective `config.h` files.

## Event Logging

The Master uses a **25LC040 (512-byte) SPI EEPROM** as a circular event log.

The external EEPROM layout reserves 8 bytes for metadata and stores up to **63 records**, each 8 bytes long. Logged event types include:

- Successful login
- Failed login
- Lockout
- Tamper detection
- Power failure
- Power restored
- Watchdog reset
- Sleep
- Wake

From a USART terminal connected to the Master:

- Send `D` or `d` to dump the log.
- Send `X` or `x` to clear the log.

## Repository Layout

```text
SmartSafe_Final/
├── README.md
├── .gitignore
├── SmartSafe.pdsprj                 # Proteus simulation project
├── master/
│   └── SmartSafeMaster/
│       ├── SmartSafeMaster.atsln    # Atmel Studio solution
│       └── SmartSafeMaster/
│           ├── main.c
│           ├── config.h
│           ├── Makefile
│           └── ...                  # Master modules
├── slave/
│   └── SmartSafeSlave/
│       ├── SmartSafeSlave.atsln     # Atmel Studio solution
│       └── SmartSafeSlave/
│           ├── main.c
│           ├── config.h
│           ├── Makefile
│           └── ...                  # Slave modules
├── Registers/
├── SmartSafe_Timer0_Timer2_Delay_Report_Package/
├── SmartSafe_Servo_Keypad_Tamper_Documents/
├── SmartSafe_Power_and_SPI_Documents/
└── SmartSafe_Complete_Project_Engineering_Report.pdf
```

## Master Firmware Modules

| Module | Purpose |
|---|---|
| `main.c` | Main state machine and system integration |
| `LCD.c/.h` | Master LCD driver |
| `keypad.c/.h` | 4×4 keypad scanning |
| `password.c/.h` | Password and failed-attempt state |
| `servo.c/.h` | Safe lock servo control |
| `adc.c/.h` | ADC support |
| `tamper.c/.h` | Tamper detection |
| `power.c/.h` | Power-failure comparator handling |
| `spi_eeprom.c/.h` | 25LC040 SPI EEPROM access |
| `logger.c/.h` | Circular event log |
| `timer.c/.h` | Timing services |
| `usart.c/.h` | Serial communication |
| `config.h` | Hardware mapping and system constants |

## Slave Firmware Modules

| Module | Purpose |
|---|---|
| `main.c` | Message processing and alarm logic |
| `LCD.c/.h` | Remote LCD driver |
| `timer.c/.h` | Alarm/status timing |
| `usart.c/.h` | Master-to-Slave serial receiver |
| `config.h` | Hardware mapping and timing constants |

## Building with Atmel Studio

The repository contains separate Atmel Studio solutions for each controller.

### Master

Open:

```text
master/SmartSafeMaster/SmartSafeMaster.atsln
```

### Slave

Open:

```text
slave/SmartSafeSlave/SmartSafeSlave.atsln
```

Both projects are configured for **ATmega32** using the AVR GCC toolchain.

Build the Master and Slave separately and use their generated firmware images for the corresponding microcontrollers in the simulation or hardware setup.

## Building with GNU Make

If AVR-GCC is installed and available on your `PATH`, each firmware can also be built from its source directory.

### Master

```bash
cd master/SmartSafeMaster/SmartSafeMaster
make
```

Output:

```text
smartsafe_master.hex
```

### Slave

```bash
cd slave/SmartSafeSlave/SmartSafeSlave
make
```

Output:

```text
smartsafe_slave.hex
```

To remove Makefile-generated objects and firmware images:

```bash
make clean
```

Required command-line tools include `avr-gcc` and `avr-objcopy`.

## Running the Proteus Simulation

1. Build both the Master and Slave firmware.
2. Open `SmartSafe.pdsprj` in Proteus.
3. Verify that each ATmega32 component points to the appropriate compiled firmware image.
4. Confirm the simulated clock frequency is **8 MHz**.
5. Start the simulation.
6. Use the keypad and simulated sensors/controls to test authentication, alarms, logging, sleep/wake, tamper detection, and power-failure behavior.

> The repository may contain old build products from development. Rebuilding the firmware before testing is recommended so Proteus uses binaries that match the current source.

## Documentation

The repository includes source `.tex` documents and rendered PDF reports covering several parts of the design, including:

- Timer0 / Timer2 timing and delay behavior
- Servo module
- Keypad module
- Tamper / ADC module
- Power module
- SPI EEPROM module
- Register/bit configuration reference
- Complete project engineering report

See `SmartSafe_Complete_Project_Engineering_Report.pdf` for the consolidated engineering documentation.

