# TFT Dash — Motorcycle Signal Reverse Engineering

This document describes the electrical signals read by the TFT Dash firmware from the motorcycle's wiring loom. Originally reverse-engineered from the firmware written for a **Yamaha FZS1000 Fazer**, with notes on differences for the **FZS600 Fazer**.

## Signal Overview

| Signal | Pin (Gen4 / Mega) | Type | Summary |
|---|---|---|---|
| Speed | 0 / 2 | Frequency (interrupt) | Pulse-width measurement from speed sensor |
| RPM | 1 / 3 | Frequency (interrupt) | Pulse-width measurement from ignition |
| Coolant Temp | A3 | Analogue (thermistor) | NTC via 2kΩ voltage divider |
| Ambient Temp | A0 | Analogue (thermistor) | NTC via 2kΩ voltage divider |
| Fuel Level | A2 | Analogue (float) | Resistive float, raw ADC |
| Battery Voltage | A4 | Analogue (divider) | 10kΩ + 5.09kΩ voltage divider |
| Light Sensor | A1 | Analogue (LDR) | Raw ADC, 0=dark 1023=bright |
| Neutral | 5 / 33 | Digital | LOW = neutral |
| Oil Level | 4 / 39 | Digital | HIGH = warning |
| High Beam | 9 / 50 | Digital | HIGH = on |
| Left Indicator | 10 / 49 | Digital | HIGH = on |
| Right Indicator | 11 / 48 | Digital | HIGH = on |
| Option Button | 8 / 43 | Digital | HIGH = pressed, 150ms debounce |
| Select Button | 7 / 44 | Digital | HIGH = pressed, 150ms debounce |
| Fan Relay | 12 / 41 | Digital OUTPUT | HIGH = fan on |
| Oil Pressure | A11 (Mega only) | Analogue (resistive) | 1004Ω voltage divider |
| Oil Temperature | A12 (Mega only) | Analogue (resistive) | 1004Ω voltage divider |

---

## Frequency-Based Signals

### Speed Sensor

The speed signal comes from the front wheel speed sensor (or gearbox-driven sensor depending on model). It produces a pulsed signal whose frequency is proportional to road speed.

**Interrupt configuration:** Triggers on `CHANGE` (both rising and falling edges), but the ISR only measures on the **falling edge** after seeing a rising edge — so it captures the full period of one complete pulse cycle.

**Valid pulse width range:** 300–60,000 µs. Anything outside this is treated as 0 speed. If no interrupt fires for >1 second, speed is set to 0.

**Conversion formula:**
```
pulses_per_second = 1,000,000 / pulse_width_µs
speed_m_s = pulses_per_second / 40.3
speed_mph = speed_m_s × 2.23694
```

**Calibration constant:** 40.3 pulses per metre (empirically determined for FZS1000).

**Smoothing pipeline:**
1. Raw pulse width captured by ISR
2. Only valid if bike out of neutral for >1.5 seconds
3. Configurable ± percentage speed correction applied
4. Floating average updated every 30ms
5. Final average over 20 samples

### RPM (Tachometer)

The RPM signal comes from the ignition system. Both the FZS600 and FZS1000 use **wasted-spark ignition with 2 coil packs** (cylinders 1&4 and 2&3), producing **2 ignition pulses per crankshaft revolution**.

**Interrupt configuration:** Same as speed — triggers on `CHANGE`, but only measures on the falling edge after a rising edge. Captures the full period of one pulse cycle.

**Valid pulse width range:** 1,500–30,000 µs. If no interrupt for >1 second, RPM is set to 0.

**Conversion formula:**
```
pulses_per_second = 1,000,000 / pulse_width_µs
rpm = pulses_per_second × RPM_CONSTANT
```

**RPM constant:** The firmware uses `29.60`, which was adjusted down from an original `33.3`. The theoretical value for 2 pulses/rev measured period-to-period is `60 / 2 = 30`, so 29.60 is close but was empirically tuned. The comment in the code is self-contradictory — it states 33.3 "was correct" but then uses 29.60.

**Smoothing pipeline:**
1. Raw pulse width captured by ISR
2. Averaged over 20 consecutive pulses in the ISR itself
3. Floating average with variable step rates (updated every 10ms):
   - ≥3000 RPM difference → step 1000
   - ≥2000 → step 500
   - ≥1000 → step 100
   - ≥500 → step 10
   - ≥250 → step 5
   - Otherwise → step 1
4. Final average over 20 samples (used for gear calculation)

**Engine running threshold:** >500 RPM averaged. Engine considered "running for 10 seconds" flag used for fan control timing.

---

## Analogue Signals

### Coolant Temperature

**Circuit:** NTC thermistor connected through a voltage divider with **R1 = 2,000Ω** (upper resistor) and **Vin = 5.1V** reference.

**Resistance measurement:**
```
Vout = (ADC_reading × 5.1) / 1023
R_thermistor = 2000 × ((5.1 / Vout) - 1)
```

**Temperature conversion (simplified Steinhart-Hart / Beta equation):**
```
t = ln(R_measured / R_stock)
T = 1 / ((1/298.15) + (t / beta)) - 273.15  →  °C
```

| Parameter | FZS1000 Value |
|---|---|
| R_stock | 30,000 Ω |
| Beta | 4,000 |

**Read interval:** Every 1 second. Averaged over 70 samples.

### Ambient / Air Temperature

Same voltage divider circuit as coolant (R1 = 2,000Ω, Vin = 5.1V). Different thermistor characteristics:

| Parameter | Value |
|---|---|
| R_stock | 10,000 Ω |
| Beta | 3,750 |

**Averaging:** 170 samples (longer window for stability since ambient changes slowly).

### Fuel Level

**Circuit:** Resistive fuel tank float sensor read as raw ADC (0–1023). Not converted to resistance — the ADC value is used directly.

**Smoothing:** Averaged over 70 samples, then a floating value that tracks ±1 per 2 seconds to prevent gauge flickering.

**Conversion to litres:** Piecewise-linear lookup table based on observed float characteristics for the FZS1000 tank (approximately 21 litres capacity). The non-linear mapping compensates for tank geometry:

| ADC Range | Litres (approx.) |
|---|---|
| 0–93 | 21 → 20 |
| 93–121 | 20 → 19 |
| 121–148 | 18 → 17 |
| 148–180 | 17 → 16 |
| 180–211 | 15 → 14 |
| 211–241 | 14 → 13 |
| 241–281 | 12 → 10 |
| 281–320 | 10 → 9 |
| 320–367 | 9 → 7 |
| 367–426 | 7 → 6 |
| 426–481 | 6 → 5 |
| 481–506 | 5 → 4 |

**Note:** This lookup table is specific to the FZS1000 tank shape and float mechanism. It will need recalibration for any other tank.

**Refuel detection:** A jump of >2 litres triggers a refuel event which resets MPG calculation.

### Battery Voltage

**Circuit:** Voltage divider with **10kΩ + 5.09kΩ** resistors stepping down the 12V battery to ADC-safe levels.

**Conversion:**
```
Vout = ADC_reading × 0.0074
V_battery = Vout × (10000 + 5090) / 5090
V_battery = Vout × 2.9685
```

**Read interval:** Every 1 second. Averaged over 70 samples.

### Light Sensor

**Circuit:** Raw analogue reading from an LDR or photodiode. 0 = dark, 1023 = bright.

**Read interval:** Every 500ms.

**Usage:** Compared against a configurable threshold (`lightswitchvalue`, range 0–900) with 20-unit hysteresis band for automatic day/night theme switching.

---

## Digital Signals

| Signal | Active State | Notes |
|---|---|---|
| **Neutral** | LOW = in neutral | Pull-down. A 1.5-second delay after leaving neutral before speed readings are accepted (prevents erroneous readings during gear changes). |
| **Oil Level Warning** | HIGH = oil low | LOW = oil level OK. |
| **High Beam** | HIGH = on | Standard active-high from headlight relay. |
| **Left Indicator** | HIGH = on | Flashing signal from indicator relay. |
| **Right Indicator** | HIGH = on | Flashing signal from indicator relay. |
| **Option Button** | HIGH = pressed | 150ms debounce. 7-second hold triggers odometer setup. |
| **Select Button** | HIGH = pressed | 150ms debounce. Confirms menu selections. |

### Fan Relay (Output)

Digital output driving a relay for an aftermarket coolant fan. HIGH = fan on. Four configurable activation modes when in neutral:

1. **Always** — Whenever in neutral with engine running >10s
2. **After 1 minute** — After being in neutral for >60s
3. **Above 50°C** — In neutral with coolant ≥50°C
4. **Throttle blip** — RPM >2,500 in neutral

Additionally, the fan **always activates** when coolant temperature exceeds a configurable threshold (range 70–120°C, default 100°C), regardless of neutral state.

---

## Calculated / Derived Values

### Gear Detection

Uses RPM/speed ratio compared against a lookup table of theoretical ratios for each gear. The theoretical speed for a given RPM and gear is:

```
shaft_speed = rpm / primary_drive_ratio
wheel_speed = shaft_speed / gearbox_ratio / final_drive_ratio
road_speed = wheel_speed × π × wheel_diameter × 60 / 1609.344  (mph)
```

A floating average tracks the current ratio with hysteresis to prevent flickering between gears. The closest match from gears 1–6 is selected.

### Trip / Odometer

Accumulated from speed × time: `distance += (speed / 3600) × time_interval_ms`. Saves to EEPROM when increment ≥ 0.1 miles.

### Fuel Economy (MPG)

```
mpg = (distance_since_refuel / litres_consumed) × 4.546
range = (mpg / 4.546) × litres_remaining
```

Uses imperial gallons (4.546 litres).

---

## Bike-Specific Constants

### FZS1000 Fazer (original firmware values)

| Parameter | Value | Source |
|---|---|---|
| Speed: pulses per metre | 40.3 | Empirical |
| RPM constant | 29.60 | Empirical (theoretical: 30) |
| Primary drive ratio | 1.581 (68/43) | Service manual |
| 1st gear | 2.500 (35/14) | Service manual |
| 2nd gear | 1.842 (35/19) | Service manual |
| 3rd gear | 1.500 (30/20) | Service manual |
| 4th gear | 1.333 (28/21) | Service manual |
| 5th gear | 1.200 (30/25) | Service manual |
| 6th gear | 1.115 (29/26) | Service manual |
| Final drive (stock) | 2.750 (44/16) | Service manual |
| Wheel diameter | 0.629m | 180/55 ZR17 rear tyre |
| Fuel tank | ~21 litres | Empirical lookup table |

### FZS600 Fazer (corrected values)

| Parameter | Value | Source |
|---|---|---|
| Speed: pulses per metre | 40.3 (TBC) | Likely similar — needs verification |
| RPM constant | 29.60 (TBC) | Same 2-coil wasted spark — may need recalibration |
| Primary drive ratio | 1.708 (82/48) | Service manual |
| 1st gear | 2.846 (37/13) | Service manual |
| 2nd gear | 1.947 (37/19) | Service manual |
| 3rd gear | 1.545 (34/22) | Service manual |
| 4th gear | 1.333 (28/21) | Service manual |
| 5th gear | 1.190 (25/21) | Service manual |
| 6th gear | 1.074 (29/27) | Service manual |
| Final drive (stock) | 3.200 (48/15) | Service manual |
| Wheel diameter | 0.624m | 160/60 ZR17 rear tyre |
| Fuel tank | ~19.4 litres | Service manual (lookup table needs recalibration) |

---

## Serial Output Protocol

### Live Data Frame

Framed by `{` and `}`, comma-delimited:
```
{,speed,rpm,coolant,battery,hour,minute,fuel,neutral,oil,highbeam,left,right,menu,info,trip1,trip2,odo,km_flag,speed_correction,theme,ambient,gear,mpg,range,maxspeed,triptime_hour,triptime_min,[oilpress_avail,oilpress,oiltemp,]fh,bar,frontsensor,rearsensor,frontpressurelow,rearpressurelow,nav_data,}
```

### Menu Data Frame

Framed by `[` and `]`, comma-delimited:
```
[,menustate,odo_digits(1-6),odo_check(1-6),time_digits(0-3),speed_correction_digits,front_sprocket,rear_sprocket,coolant_fan_temp,km,fh,bar,front_sensor,rear_sensor,front_pressure_low,rear_pressure_low,control_layout,day_theme,night_theme,light_level,light_threshold,fan_neutral_option,gear_ratio_interval,]
```

Both are sent continuously at 115200 bps over USB serial.
