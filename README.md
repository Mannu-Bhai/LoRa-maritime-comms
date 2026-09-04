# LoRa-maritime-comms
Low-cost off-grid LoRa communication for small fishing fleets beyond cellular coverage.

# Project name: RelayDrop

### A low-cost, off-grid communication network for small fishing fleets using boat-to-boat LoRa relays.

Small fishing boats can travel beyond reliable cellular coverage while the crew still carries ordinary smartphones.

RelayDrop gives those phones a way to send short messages without relying on cellular service.

A small RelayDrop node installed on each participating boat creates a local Wi-Fi network for the crew's phone. Messages are then sent over LoRa and relayed from boat to boat until they reach a shore gateway.

Think of it like passing a note from boat to boat until the note reaches land.

## How it works

Phone → Boat Node → Nearby Boat → Nearby Boat → Shore Gateway → Dashboard

- **Phone → boat:** local Wi-Fi
- **Boat → boat:** LoRa
- **Boat → shore:** LoRa relay
- **Shore → dashboard:** normal local/network connection

If no relay is available, the message can be stored and forwarded later when another RelayDrop node comes into range.

## Why RelayDrop?

The crew should not need to understand LoRa, configure radios, or install specialist software.

The cooperative installs the hardware.

The crew connects and sends.
