# Dash System Arduino Core

## Introduction
This is an Arduino implementation for the System processor on the Hologram Dash.
Build your own image using the Arduino IDE or build the included Hologram
firmware.

## Arduino Installation

1. Find your local Arduino directory (e.g. macos `~/Documents/Arduino/`).
2. In the `hardware` directory, clone this repository.
3. Open the Arduino IDE (close/reopen if already running).
4. Select the `Dash System` board from the `system.hologram.io Dash and DashPro Boards` section of the board selection menu.

## Building Firmware

1. With the `Dash System` board selected, from the Examples menu, select an example sketch.
2. Click the Verify button to build.

## Programming the User processor

1. Use Hologram Arduino package version 0.10.4 or later.
2. Open the user sketch to program.
3. From the Programmer select `hologram.io USB loader`.
4. From the Firmware Updates menu, select `Custom Firmware`.
5. Click the Upload button in the Arduino IDE.

## Programming the System processor

1. From the Programmer select `system.hologram.io USB`.
2. Connect the Dash to your computer via USB.
3. Press the Program button on the Dash.
4. Click the Upload button in the Arduino IDE.

## Support
Please feel free to [reach out to us](mailto:support@hologram.io) if you have any questions/concerns.
