# TurnoutSignals module

Mapping 14 groups of 3 LEDs (R/Y/G) to MQTT topics and PCA95x5 expanders.

- Topic format: `trains/track/turnout/<globalId>`
- `globalId = 20 + BOARD_ID*100 + localLedId`
  - `BOARD_ID` from `include/settings.h`
  - `localLedId` 1..42 (group 1 => 1-3, group 2 => 4-6, ... group 14 => 40-42)
- Payloads: `CLOSED` turns LED ON, `THROWN` turns LED OFF (active LOW)

Expanders (0x20, 0x21, 0x22) and pin map (port names P00..P17):

- Group 1: R=0x20/P06 (id 1), Y=0x20/P07 (id 2), G=0x20/P02 (id 3)
- Group 2: R=0x20/P03 (id 4), Y=0x20/P04 (id 5), G=0x20/P05 (id 6)
- Group 3: R=0x20/P01 (id 7), Y=0x20/P00 (id 8)
- Group 4: R=0x20/P12 (id 9), Y=0x20/P13 (id 10), G=0x20/P14 (id 11)
- Group 5: R=0x20/P15 (id 12), Y=0x20/P11 (id 13), G=0x20/P10 (id 14)
- Group 6: R=0x20/P09 (id 15), Y=0x20/P08 (id 16)
- Group 7: R=0x21/P14 (id 17), Y=0x21/P15 (id 18), G=0x21/P12 (id 19)
- Group 8: R=0x21/P13 (id 20), Y=0x21/P08 (id 21), G=0x21/P09 (id 22)
- Group 9: R=0x21/P10 (id 23), Y=0x21/P11 (id 24), G=0x21/P07 (id 25)
- Group 10: R=0x21/P06 (id 26), Y=0x21/P05 (id 27), G=0x21/P04 (id 28)
- Group 11: R=0x21/P02 (id 29), Y=0x21/P03 (id 30), G=0x21/P01 (id 31)
- Group 12: R=0x21/P00 (id 32), Y=0x21/P12 (id 33), G=0x22/P13 (id 34)
- Group 13: R=0x22/P14 (id 35), Y=0x22/P15 (id 36), G=0x22/P11 (id 37)
- Group 14: R=0x22/P10 (id 38), Y=0x22/P09 (id 39), G=0x22/P08 (id 40), extra ids 41 (0x22/P07) and 42 (0x22/P06)

Helpers:
- `TurnoutSignals::publishAll()` publishes all LED topics with a payload (e.g., `THROWN` at boot).
- State caching avoids redundant I2C writes when the requested LED state hasn’t changed.
