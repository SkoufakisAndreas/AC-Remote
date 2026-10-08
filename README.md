# AC Remote

A custom air conditioner remote built with **STM32F411 and Python**, based on reverse engineering the IR signals from the original remote control.

## How It Started

I captured signals from the original remote while experimenting with different combinations of temperature, operating mode, fan speed, and PURE settings. By decoding and comparing the resulting bitstrings, I identified the message structure, setting fields, and XOR checksum.

## What I Built

Using those findings, I built an STM32 remote that constructs and transmits IR messages to control the air conditioner. A Python command-line interface sends commands to the board over a serial connection.

The firmware generates a roughly **38 kHz IR carrier** and transmits **9-byte messages**, with each message repeated twice. The interface includes power control, temperature adjustment, operating modes, fan speeds, and the PURE setting.

After encountering problems with a ready-made IR transmitter module, I built my own **custom IR transmitter using a transistor and an IR LED**.

## Project Structure

| Directory | Purpose |
| --- | --- |
| `AC Remote IR Capture and Decoder/` | STM32 signal capture firmware and Python IR decoder |
| `AC_REMOTE/ac_remote/` | STM32 firmware for constructing and transmitting IR commands |
| `AC_REMOTE/remote_interface/` | Python serial command-line interface |

## Tools and Hardware

- STM32F411RE / Nucleo board
- IR receiver for capturing the original remote's signals
- Custom transistor-based IR transmitter
- STM32 HAL, STM32CubeMX, and Keil µVision
- Python and `pyserial`
