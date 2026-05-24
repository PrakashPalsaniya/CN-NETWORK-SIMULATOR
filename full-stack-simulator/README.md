# Full Stack Network Simulator

This is a fresh C++ implementation of a modular network simulator built for the ITL351 semester project. It keeps the feature set intentionally minimal and required-focused while still running traffic through the full protocol stack in one simulation flow.

## What it includes

- Physical layer: end devices, links, hubs, topology creation, frame transmission
- Data link layer: bridge, switch, MAC learning, CSMA/CD, parity error control, Go-Back-N based transmission flow
- Network layer: ARP, longest-prefix route lookup, static routing, RIP-style dynamic routing, routing table output
- Transport layer: well-known and ephemeral ports, segmented delivery, ACK handling, per-flow Go-Back-N sender state
- Application layer: two services, `echo` and `file`
- Clean console logs for each layer and domain report summaries

## Folder structure

- `include/devices`, `src/devices`: host, hub, switch, bridge, router
- `include/physical`, `src/physical`: point-to-point link behavior
- `include/analysis`, `src/analysis`: collision and broadcast domain reporting
- `include/protocols`, `src/protocols`: CSMA/CD, parity, Go-Back-N
- `include/app`, `src/app`: application services
- `include/core`, `src/core`: shared models and logging
- `include/simulator`, `src/simulator`: scenario orchestration

## Scenarios

1. Two hosts with a direct point-to-point link
2. Five hosts connected to one hub
3. Five hosts connected to one switch
4. Two five-host hub stars connected through one switch
5. Full stack routed communication with bridge, switches, two routers, ARP, RIP, and application services
6. Run every scenario

Each scenario runs the whole available stack automatically. The user selects a scenario, not an individual layer.

## Build

### With CMake

```powershell
cmake -S . -B build
cmake --build build
```

### With g++

```powershell
g++ -std=c++17 -Iinclude src/main.cpp src/app/Services.cpp src/core/Logger.cpp src/core/NetworkElements.cpp src/protocols/Protocols.cpp src/simulator/Scenarios.cpp -o full_stack_network_simulator.exe
```

## Run

```powershell
.\full_stack_network_simulator.exe
```

Or, when using CMake:

```powershell
.\build\full_stack_network_simulator.exe
```
