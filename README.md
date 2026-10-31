# ESP8266-DS18B20-MQTT-SENSOR — publish DS18B20 probe temperatures over MQTT

IoT example by [Fluidwire](https://fluidwire.com/).

Reads one or more DS18B20 waterproof temperature probes on a single 1-Wire pin every 30 seconds and publishes the readings as JSON to an MQTT broker. A low-cost node for aquaculture ponds, water tanks and incubator prototypes.

## What it does

- Connects to WiFi using credentials in `config.h`
- Connects to an MQTT broker (anonymous or username/password)
- Every 30 s, reads all DS18B20 probes on the bus (up to 8) and publishes JSON: `{"count":2,"temps_c":[27.4,27.9],"uptime_s":120}`
- Skips disconnected probes instead of publishing -127 °C
- Auto-reconnects on WiFi or MQTT loss

## Required hardware

- 1x ESP8266 board (NodeMCU 1.0, Wemos D1 mini, or any ESP8266 with WiFi)
- 1x or more DS18B20 probes (waterproof version works well)
- 1x 4.7 kΩ resistor (pull-up)
- Jumper wires, breadboard

## Required libraries

Install via the Arduino IDE Library Manager (**Sketch → Include Library → Manage Libraries…**):

- **OneWire** by Paul Stoffregen
- **DallasTemperature** by Miles Burton
- **PubSubClient** by Nick O'Leary

You also need the ESP8266 core installed via **Boards Manager**: add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to *Preferences → Additional Boards Manager URLs*, then search for `esp8266` by ESP8266 Community.

## Wiring

| DS18B20 wire    | ESP8266 pin                         |
|-----------------|-------------------------------------|
| Red (VDD)       | 3V3                                 |
| Black (GND)     | GND                                 |
| Yellow (DATA)   | GPIO4 / D2 (+ 4.7k pull-up to 3V3)  |

Additional probes connect in parallel to the same three wires; each DS18B20 has a unique 64-bit address, so they share one pin. On NodeMCU / Wemos D1 mini the label `D2` maps to `GPIO4`; the sketch uses the GPIO number.

## Setup

1. Copy `config.h.example` to `config.h`.
2. Edit `config.h` with your WiFi SSID/password and MQTT broker details. The defaults point at the public HiveMQ broker (anonymous, port 1883) — fine for first-light testing.
3. In the Arduino IDE, select **Tools → Board → NodeMCU 1.0 (ESP-12E Module)** (or your specific ESP8266 board) and the correct COM port.
4. Click **Upload**.

## Expected output

Open **Tools → Serial Monitor**, baud rate **115200**:

```
1-Wire: found 2 DS18B20 probe(s)
WiFi: connecting to your-wifi-ssid .... OK, ip=192.168.1.42
MQTT: connecting to broker.hivemq.com:1883 as fluidwire-esp8266-ds18b20 ... OK
MQTT pub fluidwire/demo/ds18b20 -> {"count":2,"temps_c":[27.4,27.9],"uptime_s":30}
```

Subscribe to verify:

```
mosquitto_sub -h broker.hivemq.com -t 'fluidwire/demo/ds18b20' -v
```

## Troubleshooting

- **`No probes found`** — missing 4.7k pull-up between DATA and 3V3, swapped wires, or the wrong GPIO.
- **Reads 85.0 °C** — the probe's power-on default; usually unstable power or a weak pull-up. Add a 100 nF capacitor near the probe or shorten the cable.
- **Probe order changes between boots** — the library indexes by bus address order; for fixed roles, look up each probe's address with `getAddress()`.
- **`MQTT: connecting ... FAIL rc=-2`** — broker host unreachable. Check WiFi got an IP and the broker hostname.
- **`MQTT: connecting ... FAIL rc=5`** — auth refused. Check the username/password.
- **WiFi connect spins forever** — wrong `WIFI_SSID`, or a 5 GHz-only network. The ESP8266 is 2.4 GHz only.

## Security note

Don't commit `config.h`. The `.gitignore` in this repo already excludes it; only `config.h.example` is tracked. For production use, prefer TLS (`WiFiClientSecure` + port 8883) and per-device credentials.

## License

MIT — see [LICENSE](LICENSE).

## Got stuck?

Drop by [fluidwire.com](https://fluidwire.com/) — we ship IoT building blocks and are happy to help.
