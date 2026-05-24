# ECO & Silent Mode — Research Findings

> **Status:** Not implementable (as of May 2025)  
> **Related upstream issue:** [absalom-muc/MHI-AC-Ctrl#219](https://github.com/absalom-muc/MHI-AC-Ctrl/issues/219)

## Summary

ECO, Silent, and Night Setback modes are **explicitly listed as unsupported** in the upstream MHI-AC-Ctrl project. The SPI protocol documentation does not specify any bits for these modes. No fork or community project has successfully implemented read or write support.

## What These Modes Do

| Mode | Behavior (from user reports & MHI manuals) |
|------|---------------------------------------------|
| **ECO** | Limits power consumption by ~30%; caps compressor output |
| **Silent** | Increases hysteresis (wider dead-band around setpoint), reduces compressor cycling and fan noise |
| **Night Setback** | After inactivity timer, automatically reduces setpoint (e.g. heat: 15°C → 10°C) |

## What Happens Today

When ECO or Silent is activated via IR remote, the AC emits **unknown operating data** that falls through to the `opdata_unknown` handler:

```
MQTT topic: MHI-AC/unknown → 4385, 4129, 32989
```

These decode as:

| Raw value | Hex      | DB9 (opcode) | DB10 (sub-code) |
|-----------|----------|--------------|-----------------|
| 4385      | `0x1121` | `0x21`       | `0x11`          |
| 4129      | `0x1021` | `0x21`       | `0x10`          |
| 32989     | `0x80DD` | `0xDD`       | `0x80`          |

**Opcode `DB9=0x21`** (decimal 33) is completely unknown to the firmware. The two sub-codes (`0x10`, `0x11`) likely distinguish ECO from Silent, but which is which is unconfirmed.

## Why It Can't Be Implemented Yet

### 1. No Known SPI Write Bits

The documented MISO (ESP→AC) set-bits cover only: Power, Mode, Fan, Vanes, and Temperature. **No MISO bit has been discovered for activating ECO or Silent mode.**

Candidate locations for undiscovered bits:
- **DB5** — entirely undocumented
- **DB13** — partially unknown ("outdoor unit status")
- **DB15–DB26** — extended 33-byte frame (used by WF-RAC adapter)

### 2. Read-Only Detection is Partially Possible

The `opdata_unknown` handler already captures ECO/Silent activity as raw numbers. With a targeted SPI trace session, one could:
- Confirm which sub-code maps to which mode
- Publish decoded status (e.g. `MHI-AC/EcoMode → on/off`)

However, without write capability, this would be **read-only**.

### 3. ESPHome "QUIET" is NOT Silent Mode

The ESPHome port's `CLIMATE_FAN_QUIET` maps to **SPI fan speed 0** (ultra-low, below level 1). This is unrelated to the MHI Silent mode button on the IR remote.

## Path to Implementation

If someone wants to pursue this:

1. **Capture SPI traces** — Use `testprog/SPI_logger.ino` while toggling ECO/Silent via IR remote. Look for changes in DB5, DB13, and the DB9=0x21 operating data.

2. **Identify the write path** — Analyze what the official MHI WiFi adapter (MH-AC-WIFI-1 / WF-RAC) sends in its MISO frames during ECO/Silent activation. This would require a 3-way SPI tap (ESP listening to both AC and official adapter).

3. **Test with extended frame** — The 33-byte frame (DB15–DB26) used by WF-RAC adapters may contain the ECO/Silent bits. Try setting various bits in these bytes and observing AC behavior.

### Equipment needed:
- Second ESP8266 running `SPI_logger.ino` (or logic analyzer)
- IR remote with ECO/Silent buttons
- Optionally: Official MH-AC-WIFI-1 adapter for MISO sniffing

## References

- Upstream known limitations: [`Troubleshooting.md`](https://github.com/absalom-muc/MHI-AC-Ctrl/blob/master/Troubleshooting.md#known-limitations)
- SPI protocol documentation: [`absalom-muc/MHI-AC-Trace:SPI.md`](https://github.com/absalom-muc/MHI-AC-Trace)
- Issue #219 (opdata clue): [absalom-muc/MHI-AC-Ctrl#219](https://github.com/absalom-muc/MHI-AC-Ctrl/issues/219)
- Issue #126 (ECO mode discussion): [absalom-muc/MHI-AC-Ctrl#126](https://github.com/absalom-muc/MHI-AC-Ctrl/issues/126)

## Model Compatibility

ECO/Silent appears to be **model-specific**:
- **SRK ZSX models** (ZSX-WFT, ZSX-W) — confirmed to have ECO/Silent on IR remote
- **SRK ZS-S** (upstream author's test unit) — may not support ECO mode
- **SRK ZS-W** (this project's unit) — needs verification via IR remote
