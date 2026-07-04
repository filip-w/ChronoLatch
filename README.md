# ChronoLatch

ChronoLatch is a solar-powered release-only gate opener and timed relay
controller for feeding horses and other low-power automation tasks.

The field design target is deliberately conservative: at a scheduled feed time
the controller releases a latch, and the gate opens by spring tension, gravity,
an offset hinge, or a low-force open-only actuator. The system does not
automatically power-close a gate around animals. The controller can read and set
both the RTC clock and the daily opening time over USB serial, and it only holds
the relay closed for 3 seconds.

![First ChronoLatch prototype](FirstPrototype.jpg)

### Prototype

Use this path to test scheduling and relay behavior on the bench.

- DS3231 RTC module.
- Arduino Nano or Pro Mini.
- CN3065 charger, 6 V solar cell, and one 18650 Li-ion cell.
- MT3608 boost converter for 5 V electronics.
- 1-channel 5 V relay module.

### Bill of Materials (BOM)

| Module / Part | Qty | Purpose | Affiliate Link |
| --- | ---: | --- | --- |
| DS3231 RTC module | 1 | Timekeeping | [AliExpress](https://s.click.aliexpress.com/e/_c43xkIFb) |
| Arduino Nano | 1 | Main controller | [AliExpress](https://s.click.aliexpress.com/e/_c4LZC2yH) |
| MT3608 boost converter | 1 | Voltage step-up regulation | [AliExpress](https://s.click.aliexpress.com/e/_c3mCP8yH) |
| CN3065 solar charger module with 6 V solar panel kit | 1 | Solar charging and power input | [AliExpress](https://s.click.aliexpress.com/e/_c3M47D7P) |
| Protected 18650 Li-ion battery | 1 | Rechargeable power storage | TBD |
| 1-channel 5 V relay module | 1 | Switched output control | [AliExpress](https://s.click.aliexpress.com/e/_c43xkIFb) |

### Overview module schematic

```mermaid
flowchart TB
    Solar["6 V solar panel<br/>1.25 W, 210 mA"]
    Charger["CN3065<br/>Li-ion solar charger"]
    Battery["Protected 18650<br/>Li-ion cell"]
    Boost["MT3608<br/>boost converter"]
    Nano["Arduino Nano<br/>ChronoLatch firmware"]
    RTC["DS3231 RTC<br/>timekeeping module"]
    Relay["1-channel 5 V relay<br/>input on D2"]
    Latch["Gate latch release<br/>3 second pulse"]
    USB["USB serial<br/>setup and status"]

    Solar -->|"charge input"| Charger
    Charger -->|"battery charge"| Battery
    Battery -->|"3.0-4.2 V"| Boost
    Boost -->|"5 V rail"| Nano
    Boost -->|"5 V rail"| RTC
    Boost -->|"5 V rail"| Relay

    RTC -->|"SDA -> A4"| Nano
    RTC -->|"SCL -> A5"| Nano
    USB <-->|"9600 baud serial"| Nano
    Nano -->|"D2 relay control"| Relay
    Relay -->|"switched output"| Latch
```

| Module | Connection |
| --- | --- |
| Relay input | Arduino `D2` |
| DS3231 SDA | Arduino `A4` |
| DS3231 SCL | Arduino `A5` |
| Power rail | MT3608 `5 V` output to Arduino, RTC, and relay module |
| Gate output | Relay contacts switch the latch release circuit |

### Serial troubleshooting

If a minimal serial sketch works but ChronoLatch prints nothing, check where the
boot log stops:

- `ChronoLatch booting...` means USB serial is alive.
- `Checking RTC...` followed by no ready message points at the DS3231/I2C bus.
  Check RTC power, ground, `SDA -> A4`, `SCL -> A5`, and pullups.
- `I2C timeout while checking RTC` means the Arduino Wire timeout recovered from
  a stuck I2C transaction.

The firmware sleeps after the initial setup window to save power. Press reset or
open the serial monitor immediately after upload when testing commands.

### Power budget

Prototype measurements after removing the Arduino Nano power LED and relay board
VCC LED:

- Sleeping controller load: about 14 mA.
- Relay active load: about 40 mA.
- Battery: 3500 mAh protected 18650 Li-ion cell.
- Solar panel: mini 6 V, 210 mA, 1.25 W.

With one 3 second release per day, the relay pulse adds very little to the daily
energy use. The sleeping controller dominates the budget:

- Daily use: about 336 mAh, or 1.24 Wh from a nominal 3.7 V cell.
- Battery-only runtime: roughly 250 hours, or 10.4 days, before allowing for
  cold weather, battery age, converter losses, and cutoff voltage.

Cloudy-day prototype measurement while the MCU was sleeping:

![ChronoLatch prototype cloudy-day battery current measurement](CurrentMeasurement.jpg)

With the solar panel connected through the charger, the battery-side current was
about 25 mA into the battery on a cloudy day. At that charge rate, the panel
would put back about 25 mAh per hour of similar light. Replacing one full day of
sleeping-controller use would therefore take about 13-14 hours at this cloudy
rate, while brighter sun should improve the margin.
