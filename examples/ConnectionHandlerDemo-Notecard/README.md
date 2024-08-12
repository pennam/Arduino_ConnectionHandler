Notecard Connectivity
=====================

The Notecard is a wireless, secure abstraction for device connectivity, that can
be used to enable _ANY*_ device with I2C, or UART, to connect to the Arduino IoT
Cloud via cellular, LoRa, satellite or Wi-Fi!

As a result, your existing device architecture may now have access to the
Arduino IoT Cloud by using a Notecard to provide a secure communication channel.

> \*_While any device with I2C/UART may use the Notecard, the AVR architecture
> is not supported by the Arduino IoT Cloud library. Therefore, those devices
> are ineligible for use with the Notecard._

Wireless Connectivity Options
-----------------------------

- [Cellular](https://shop.blues.com/collections/notecard/products/notecard-cellular)
- [Cellular + Wi-Fi](https://shop.blues.com/collections/notecard/products/notecard-cell-wifi)
- [Wi-Fi](https://shop.blues.com/collections/notecard/products/wifi-notecard)
- [LoRa](https://shop.blues.com/collections/notecard/products/notecard-lora)
- [Satellite](https://shop.blues.com/products/starnote)

How it Works
------------

**Architecture Diagram:**

```
--------                ------------                  -----------           -----------
|      |                |          |                  |         |           |         |
| Host |                |          |      Secure      |         |           | Arduino |
| MCU  |---<I2C/UART>---| Notecard | ( ( Wireless ) ) | Notehub |---<TLS>---|   IoT   |
|      |                |          |     Protocol     |         |           |  Cloud  |
|______|                |__________|                  |_________|           |_________|
```

Getting Started
---------------

- Quick Start Guide: https://dev.blues.io/quickstart/
