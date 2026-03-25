# Neiry Band → OSC → TouchDesigner

This code reads EEG data from a **Neiry Band** headset, processes it in real time, and streams the results to **TouchDesigner** (or any OSC-capable software).

Built for artists and performers — not neuroscientists.  
No academic rigor. Just signal you can touch.

---

## Output (OSC channels)

All data is sent to `127.0.0.1:8000`

| address | type | description |
|---|---|---|
| `/neiry/arousal` | float 0–1 | **most responsive channel** — instant neural activity: facial tension, focus, movement |
| `/neiry/ratio` | float 0–1 | relaxation: 1.0 = calm, 0.0 = tension |
| `/neiry/blink_env` | float 0–1 | blink envelope: jumps to 1.0 on blink, decays over ~350ms |
| `/neiry/blink` | int 1 | momentary blink trigger |
| `/neiry/alpha` | float 0–1 | alpha rhythm (8–13 Hz) — rises when eyes are closed |
| `/neiry/beta` | float 0–1 | beta rhythm (14–30 Hz) — active thinking, tension |
| `/neiry/theta` | float 0–1 | theta rhythm (4–7 Hz) — meditation, drowsiness |
| `/neiry/hb` | int N | heartbeat counter (use to verify signal is flowing) |

> Signals stabilize after roughly 8–10 seconds of wearing the headset.  
> Quick check for ratio: close your eyes for 5 seconds — the value should drift upward.

---

## Requirements

1. **Neiry Band** headset + Bluetooth on your computer
2. **Capsule SDK** — download from Neiry (version 1.3.0 or later)  
   After installation, the folder structure looks like this:
   ```
   D:\Capsule_and_API_1_3_0_22_03_2024\
     capsule-public-v1.3.0\
       CapsuleAPI\
         Win\
           Source\      ← put the files here
           build\
   ```
3. **Visual Studio** (free Community edition) with C++ support
4. **TouchDesigner** or any other software that receives OSC

---

## Installation — step by step

### Step 1 — copy the files

Download the three files from this repository and place them in:
```
CapsuleAPI\Win\Source\
```

Files:
- `main.cpp` (this is RawSignalExample.cpp — rename if needed)
- `EEGBands.hpp`
- `OscSender.hpp`

### Step 2 — open the project in Visual Studio

Open the `.sln` or `CMakeLists.txt` file from:
```
CapsuleAPI\Win\build\
```

### Step 3 — build

In Visual Studio: **Build → Build Solution** (or `Ctrl+Shift+B`)

The compiled `.exe` will appear in:
```
CapsuleAPI\Win\build\build\Release\
```

### Step 4 — run

Launch `CapsuleRawSignalExample.exe`  
The program will ask: **Write raw data to CSV? (y/n)** — press `n` unless you need it.

OSC data will start flowing to port 8000.

---

## TouchDesigner — receiving the signal

1. Add an **OSC In CHOP** node
2. Set: Network Address `127.0.0.1`, Port `8000`
3. In parameters set **Scope: /neiry/***

Channels will appear automatically once data starts coming in.

---

## Files in this repository

| file | description |
|---|---|
| `main.cpp` | main program — Neiry connection, filtering, OSC output |
| `EEGBands.hpp` | filters: biquad BPF, blink detector, auto-normalization |
| `OscSender.hpp` | OSC over UDP (Windows) |

---

## Author

[Sasha Snova](https://sashasnova.art) — media artist, Belgrade  
Developed for the performance *Subconscious Negativity* (BYOB Belgrade, 2026)

Questions, bugs, forks — welcome.
