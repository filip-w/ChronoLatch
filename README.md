# ChronoLatch

ChronoLatch is a solar-powered release-only gate opener and timed relay
controller for feeding horses and other low-power automation tasks.

The field design target is deliberately conservative: at a scheduled feed time
the controller releases a latch, and the gate opens by spring tension, gravity,
an offset hinge, or a low-force open-only actuator. The system does not
automatically power-close a gate around animals. The controller can read and set
both the RTC clock and the daily opening time over USB serial, and it only holds
the relay closed for 10 seconds.

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
    Latch["Gate latch release<br/>10 second pulse"]
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

### Power budget

After removing the Arduino Nano power LED and relay board VCC LED:

- Sleep current: about 14 mA.
- Awake with relay active: about 40 mA.
- Battery: 3500 mAh 18650 Li-ion cell.
- Solar panel: mini 6 V, 210 mA, 1.25 W.

With one 10 second release per day, average draw is about 14.003 mA. Battery-only
runtime is roughly 250 hours, or 10.4 days, before allowing for cold weather,
battery age, converter losses, and cutoff voltage.

Daily use is about 336 mAh, or 1.24 Wh from a nominal 3.7 V cell. The panel can
replace that in about one ideal peak-sun hour, but plan for 2-3 peak-sun hours
per day in real conditions.
