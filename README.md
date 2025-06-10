# Monte Carlo Option Pricing (C++)

## Overview

This project implements a modular and extensible **Monte Carlo simulation framework** for option pricing using modern C++. It supports various numerical methods (Euler, Milstein, Predictor-Corrector, etc.) and stochastic models (GBM, CEV), with reusable components for simulation, pricing, and random number generation.

>  This C++ implementation is ported from a C# version (e.g., `MCBuilder.cs`, `Pricers.cs`, `SDE.cs`).

---

## Project Structure

| File                | Description |
|---------------------|-------------|
| `Main.cpp`          | Entry point that runs the simulation via `MCPricerApplication` |
| `OptionData.hpp`    | Holds option parameters and returns callable payoff/discount functions |
| `SDE.hpp`           | Interface and concrete SDE models (`GBM`, `CEV`) |
| `Fdm.hpp`           | Finite difference schemes (Euler, Milstein, PC variants, Platen, Heun, etc.) |
| `Pricers.hpp`       | Defines pricer classes for European, Asian, and Barrier options |
| `Rng.hpp`           | Random number generators: Box-Muller, Mersenne Twister, Marsaglia |
| `StopWatch.hpp`     | Simple stopwatch to time the simulation |
| `MCBuilder.hpp`     | Builder classes for configuring and assembling simulation components |
| `MCMediator.hpp`    | Coordinates path generation and dispatch via signal-slot mechanism |

---

## Build Instructions

### Dependencies:
- C++11/14/17 or later
- Boost (for Signals2)

---

## How to Run

```bash
./OptionsPricing/OptionsPricing/Test.cpp
```

### Sample interaction:

```
Set S_0:
100
How many NSim?
10000

Select a Monte Carlo builder implementation:
1. Use MCBuilder
2. Use MCDefaultBuilder
1

Create SDE
1. GBM, 2. CEV
1

Create RNG
1. Box-Muller .Net 2. My Mersenne Twister 3. Polar Marsaglia .Net
1

Create FDM
1. Euler, 2. Milstein, ...
1
How many NT?
100

Compute Plain price:
Price, #Sims : 10.4176, 10000
Time elapsed: 0.14s
```

---

## Features

- Modular architecture with plug-and-play components
- Interactive factory for building SDE + FDM + RNG combinations
- Clean signal-slot architecture using Boost.Signals2
- Supports GBM and CEV processes
- Supports multiple finite difference methods
- Reusable pricers (European, Asian, Barrier, Brownian Bridge logic)
- Easy to extend for other stochastic models or option types

---

## Extensibility

To add new models or methods:

| Component | Extend This Class      |
|----------|------------------------|
| SDE      | `ISde`                 |
| FDM      | `FdmBase`              |
| RNG      | `IRng`                 |
| Pricer   | `IPricer`              |

Then wire them up in `MCBuilder`.

---


