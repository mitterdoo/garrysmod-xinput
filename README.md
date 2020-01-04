# garrysmod-xinput
* [Installation](#installation)
* [Getting it to work](#getting-it-to-work)
* [Hooks](#hooks)
  * [xinputConnected](#xinputconnectedid-when)
  * [xinputDisconnected](#xinputdisconnectedid-when)
  * [xinputPressed](#xinputpressedid-button-when)
  * [xinputReleased](#xinputreleasedid-button-when)
  * [xinputTrigger](#xinputtriggerid-value-trigger-when)
  * [xinputStick](#xinputstickid-x-y-stick-when)
* [Functions](#functions)
  * [xinput.getState(`id`) → `XINPUT_GAMEPAD`/`false`](#xinputgetstateid--xinput_gamepadfalse)
  * [xinput.getButton(`id`, `button`) → `bool`](#xinputgetbuttonid-button--bool)
  * [xinput.getTrigger(`id`, `trigger`) → `number`](#xinputgettriggerid-trigger--number)
  * [xinput.getStick(`id`, `stick`) → `number`, `number`](#xinputgetstickid-stick--number-number)
  * [xinput.getBatteryLevel(`id`) → `number`/`false`, `string`](#xinputgetbatterylevelid--numberfalse-string)
  * [xinput.getControllers() → `table`](#xinputgetcontrollers--table)
  * [xinput.setRumble(`id`, `softPercent`, `hardPercent`)](#xinputsetrumbleid-softpercent-hardpercent)
* [Enums](#enums)
  * [XINPUT\_GAMEPAD\_*](#xinput_gamepad_)
* [Structs](#structs)
  * [XINPUT_GAMEPAD](#xinput_gamepad)

A Lua binary module for Garry's Mod that exposes XInput (1.3) functions
This hooks into Garry's Mod's `xinput1_3.dll` that is provided with any build.

XInput is far better than DirectInput (the alternative of getting controller input in GLua), because DirectInput maps both triggers to one axis, making them indistinguishable if both are in use. XInput does not have this issue. Additionally, XInput has rumble support.
Finally, this module has a threaded poll loop. That means that if your game freezes momentarily, your inputs will still be registered! They will also record the instant that they occurred.

## Installation
If you are running Garry's Mod in 32-bit (this is **not** asking about your machine architecture; Garry's Mod is a 32-bit game and, at the time of writing, become 64-bit if and only if you are on the x86-64 branch), place `gmcl_xinput_win32.dll` in `garrysmod/garrysmod/lua/bin`. If the `bin` folder does not exist, create one. If you're running 64-bit, place `gmcl_xinput_win64.dll` in the _same location_.

## Getting it to work
This module ***will not work properly*** if Steam (or some other program) is *eating* your controller inputs. This will happen when Steam takes your controller inputs, and maps them to keys and mouse movements (instead of actual controller inputs). In order to fix this, you must use the "Gamepad" controller configuration template. This will pipe your controller inputs into XInput. To enable this, enter the "Controller configuration" menu while in-game, select "BROWSE CONFIGS", go into "Templates", and select "Gamepad."
![Steam Screenshot](http://mitterdoo.net/u/2019-06/3c605d5d-8966-4a75-a9dd-2374312b3bd9.png)
You can also set up *any* controller detected by Steam to act as an XInput controller, by binding each button to the actual button you want to be pressed in XInput.


# Functionality
This module adds the following hooks and functions:
## Hooks
_For all events, `when` is the `RealTime()` at which it occurred._

### xinputConnected(`id`, `when`)
Called when controller number `id` has been connected. *Note: to save on computing time, controllers that are disconnected will only be polled every 4 seconds. It may take longer for the reconnect to be detected.*

### xinputDisconnected(`id`, `when`)
Called when controller number `id` has been disconnected.

### xinputPressed(`id`, `button`, `when`)
Called when `button` _(see [XINPUT_GAMEPAD_*](#xinput_gamepad_) enums)_ on controller number `id` has been pushed down.

### xinputReleased(`id`, `button`, `when`)
Called when `button` _(see [XINPUT_GAMEPAD_*](#xinput_gamepad_) enums)_ on controller number `id` has been released.

### xinputTrigger(`id`, `value`, `trigger`, `when`)
Called when `trigger` (0 is left) on controller number `id` has been moved to new position `value`. `value` is between 0-255 inclusive.

### xinputStick(`id`, `x`, `y`, `stick`, `when`)
Called when `stick` (0 is left) on controller number `id` has been moved to new coordinates (`x`, `y`). Each coordinate, `x`, and `y`, are between -32768 - 32767 inclusive.

## Functions

### xinput.getState(`id`) → [`XINPUT_GAMEPAD`](#xinput_gamepad)/`false`
Gets the state of controller number `id`. This returns a [XINPUT_GAMEPAD](#xinput_gamepad) struct, or `false` if the controller is disconnected.

### xinput.getButton(`id`, `button`) → `bool`
Gets whether the `button` _(see [XINPUT_GAMEPAD_*](#xinput_gamepad_) enums)_ on controller `id` is currently pushed down.

### xinput.getTrigger(`id`, `trigger`) → `number`
Gets the current position of `trigger` (0 is left) on controller `id`. Returns a value between 0-255 inclusive.

### xinput.getStick(`id`, `stick`) → `number`, `number`
Gets the current coordinates of `stick` (0 is left) on controller `id`. Each coordinate is between -32768 - 32767 inclusive.

### xinput.getBatteryLevel(`id`) → `number`/`false`, `string`
Attempts to get the current battery level of controller `id`. If successful, returns the level between 0.0-1.0 inclusive. Otherwise, returns `false`, and a message explaining why it couldn't get the level.

### xinput.getControllers() → `table`
Returns a table where each _key_ is the `id` of a controller that is currently connected (the values are always `true`).

### xinput.setRumble(`id`, `softPercent`, `hardPercent`)
Sets the rumble on controller `id`. Each percent value is between 0.0-1.0 inclusive. XInput controllers have two rumble motors: a soft motor, and a hard motor. This allows for many different combinations of how the rumble "feels".

## Enums

### XINPUT_GAMEPAD_*
Name | Value (hexadecimal)
-----|--------------------
XINPUT_GAMEPAD_DPAD_UP | 0x0001
XINPUT_GAMEPAD_DPAD_DOWN | 0x0002
XINPUT_GAMEPAD_DPAD_LEFT | 0x0004
XINPUT_GAMEPAD_DPAD_RIGHT | 0x0008
XINPUT_GAMEPAD_START | 0x0010
XINPUT_GAMEPAD_BACK | 0x0020
XINPUT_GAMEPAD_LEFT_THUMB | 0x0040
XINPUT_GAMEPAD_RIGHT_THUMB | 0x0080
XINPUT_GAMEPAD_LEFT_SHOULDER | 0x0100
XINPUT_GAMEPAD_RIGHT_SHOULDER | 0x0200
XINPUT_GAMEPAD_A | 0x1000
XINPUT_GAMEPAD_B | 0x2000
XINPUT_GAMEPAD_X | 0x4000
XINPUT_GAMEPAD_Y | 0x8000

## Structs

### XINPUT_GAMEPAD
Type | Name | Description
-----|------|------------
`number` | `wButtons` | A bitmask containing all buttons pushed. See [XINPUT_GAMEPAD_*](#xinput_gamepad_) to see which bits correspond to which button.
`number` | `bLeftTrigger` | The position of the left trigger, between 0-255 inclusive.
`number` | `bRightTrigger` | The position of the right trigger, between 0-255 inclusive.
`number` | `sThumbLX` | The X-coordinate of the left thumb stick, between -32768 - 32767 inclusive.
`number` | `sThumbLY` | The Y-coordinate of the left thumb stick, between -32768 - 32767 inclusive.
`number` | `sThumbRX` | The X-coordinate of the right thumb stick, between -32768 - 32767 inclusive.
`number` | `sThumbRY` | The Y-coordinate of the right thumb stick, between -32768 - 32767 inclusive.
