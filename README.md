# Kryppi – Encrypted Mesh Chat

Kryppi is a peer-to-peer chat application for Raspberry Pi nodes connected through a **batman-adv** wireless mesh. Messages are sent as UDP broadcasts, forwarded hop-by-hop by every node, encrypted with a per-chat key, and shown in an **ncurses** terminal UI with delivery receipts.

No server, router or internet connection is needed – the Pis talk directly to each other over Wi-Fi in ad-hoc mode.

```
 ┌────────┐   wlan0 (IBSS)   ┌────────┐   wlan0 (IBSS)   ┌────────┐
 │  Pi A  │ ◄──────────────► │  Pi B  │ ◄──────────────► │  Pi C  │
 │ kryppi │      bat0        │ kryppi │      bat0        │ kryppi │
 └────────┘   10.0.x.y/16    └────────┘                  └────────┘
        UDP broadcast to 10.0.255.255:8888, re-broadcast by each node
```

## Features

- **Mesh networking** – batman-adv on `wlan0` in ad-hoc mode; every node gets a `10.0.x.y` address derived from its MAC
- **Flooding with de-duplication** – packets carry a random 32-bit ID; each node forwards packets not addressed to it and drops IDs it has already seen
- **Per-chat encryption keys** – messages are serialised to JSON, XOR-encrypted with the chat's key and hex-encoded
- **Delivery receipts** – `✓` when the receiver got the packet (`ACK_RECV`), `✓✓` when it decrypted it with the right key (`ACK_DECRYPTED`)
- **Locked chats** – incoming chats are stored encrypted and must be unlocked with the correct key before they can be read
- **Terminal UI** – splash screen, menu, chat list, new-chat dialog, chat view with scrolling, setup screen

## Repository structure

```
.
├── BATMAN/                  # Mesh setup for Raspberry Pi
│   ├── setup-mesh.sh        # Configure node, install batctl, create systemd service, reboot
│   ├── cleanup-mesh.sh      # Undo everything and return wlan0 to NetworkManager
│   ├── demo-visuals.sh      # Visual-only walkthrough of the setup (no changes made)
│   └── README.md            # Detailed mesh setup guide
├── udp-socket/              # Networking, encryption, JSON
│   ├── inc/
│   │   ├── config.hpp       # Port, buffer size, broadcast address, default key
│   │   ├── udp_socket.hpp   # MeshPacket + MeshNode
│   │   ├── kryp.hpp         # XOR + hex helpers
│   │   ├── json.h           # jsonwriteread (message (de)serialisation)
│   │   └── json.hpp         # nlohmann/json (single header)
│   ├── src/
│   │   ├── udp_socket.cpp   # Socket, flooding, ACKs, send
│   │   ├── kryp.cpp
│   │   ├── json.cpp         # JSON + reads node IP from ~/mesh/node_ip.txt
│   │   └── main.cpp         # Stand-alone CLI test client (mesh_test)
│   └── Makefile
└── tui/                     # ncurses client (kryppi)
    ├── include/             # draw_tui.h, run_tui.h, msg_handler.h
    ├── src/
    │   ├── main.cpp         # Wires MeshNode ↔ TUI, starts network thread
    │   ├── run_tui.cpp      # Main loop: draw + input per screen
    │   ├── draw_tui.cpp     # All screens and key handling
    │   └── msg_handler.cpp  # Incoming messages and ACKs → chat state
    └── CMakeLists.txt
```

## Requirements

**Hardware:** two or more Raspberry Pis with built-in Wi-Fi (`wlan0`).

**Software (on each Pi):**

- Raspberry Pi OS with NetworkManager
- `g++` (C++17), `cmake` ≥ 3.15, `make`
- ncurses with wide-character support: `sudo apt install libncurses-dev`
- `batctl` (installed by `setup-mesh.sh`)

## Getting started

### 1. Join the mesh

On **each** Pi:

```bash
cd BATMAN
chmod +x setup-mesh.sh
./setup-mesh.sh
```

The script derives the node IP from the last two bytes of the `wlan0` MAC (`10.0.<b5>.<b6>`), writes it to `~/mesh/node_ip.txt`, configures `wlan0` for the mesh (`my-mesh`, channel 1), installs a `batman-mesh` systemd service so the mesh starts at boot, and **reboots**.

After reconnecting, check:

```bash
ip addr show bat0    # 10.0.x.y/16
sudo batctl n        # neighbouring nodes
```

See [BATMAN/README.md](BATMAN/README.md) for details and `cleanup-mesh.sh` to undo the setup.

### 2. Build the chat client

```bash
cd tui
cmake -S . -B build
cmake --build build
```

### 3. Run

```bash
./build/kryppi
```

The client reads its own IP from `~/mesh/node_ip.txt`. Run it on two nodes, create a chat with the other node's IP, and use the same key on both sides.

> Starting with any argument (e.g. `./build/kryppi demo`) launches the UI **without networking** – handy for testing the interface on a laptop.

## Using the TUI

| Screen | Keys |
|--------|------|
| Menu | `↑`/`↓` navigate · `Enter` select · `q` quit |
| Chats | `↑`/`↓` select · `Enter` open · `N` new chat · `Esc` back |
| New chat | type IP · `Tab` switch to key field · `Enter` connect · `Esc` cancel |
| Unlock chat | type key · `Enter` unlock · `Esc` cancel |
| Chat | type message · `Enter` send · `↑`/`↓` scroll · `Esc` back |
| Setup | shows protocol and own mesh IP · `Esc` back |

- If the key field is left empty in *New chat*, the default key from `config.hpp` is used.
- A chat started by someone else appears in the list as **locked**. Opening it asks for the key, which is checked by trying to decrypt the latest received message.
- Sent messages show `✓` (received) and `✓✓` (received and decrypted with the right key).

## How it works

### Packet format

Every message is one fixed-size UDP datagram on port **8888**:

```cpp
struct MeshPacket {
    uint32_t messageId;     // random ID used for de-duplication and ACKs
    char     targetIp[16];
    char     senderIp[16];
    char     payload[512];  // hex-encoded ciphertext, or "ACK_RECV:<id>" / "ACK_DECRYPTED:<id>"
};
```

### Sending

1. `jsonwrite()` builds `{"modtager_ip": ..., "besked": ..., "tid": <unix time>}`
2. The JSON is XOR-encrypted with the chat key and hex-encoded (`kryp`)
3. Packets whose payload would be ≥ 512 characters are rejected
4. The packet gets a new ID and is broadcast to `10.0.255.255:8888`

### Receiving and forwarding

`MeshNode::receiveAndSend()` runs on a background thread:

1. Drop the packet if its ID has already been seen (the last ~100 000 IDs are kept)
2. If `targetIp` isn't this node, **re-broadcast** it unchanged – this is what carries messages across multiple hops
3. If it's for this node:
   - ACK payloads update the matching sent message in the UI
   - otherwise reply with `ACK_RECV`, hex-decode, and pass the ciphertext to the TUI
   - if it decrypts to valid JSON with the chat's key, reply with `ACK_DECRYPTED`

Received messages are stored encrypted in memory and decrypted when drawn, using the key entered for that chat.

### Logging

`json.cpp` appends sent messages to `write.json` and received messages to `read.json` in the working directory.

## Stand-alone test client

`udp-socket/` also builds a simple CLI client without the TUI, useful for testing networking on its own:

```bash
cd udp-socket
make
./build/bin/mesh_test [ip] [broadcast-address]
```

Without arguments it reads the IP from `~/mesh/node_ip.txt`. To test two instances on one machine, use loopback, e.g. `./build/bin/mesh_test 10.0.0.1 127.255.255.255` in one terminal and `./build/bin/mesh_test 10.0.0.2 127.255.255.255` in another. Note that this client always decrypts incoming messages with the default key from `config.hpp`.

## Configuration

`udp-socket/inc/config.hpp`:

| Setting | Default |
|---------|---------|
| `PORT` | `8888` |
| `BROADCAST_ADDR` | `10.0.255.255` |
| `BUFFER_SIZE` | `1024` |
| `CRYPTO_KEY` | default key for new chats without a key |

Mesh settings (SSID `my-mesh`, frequency 2412 MHz, MTU 1468, `/16` subnet) are in `BATMAN/setup-mesh.sh`.

## Security note

This is an educational project. A repeating-key XOR cipher is **not** secure – it is easy to break with known plaintext (every message starts with the same JSON structure), and there is no authentication, so sender IPs can be spoofed. Don't use it for anything sensitive.

## Known limitations

- Linux only (POSIX sockets, ncurses); `setup-mesh.sh` assumes Raspberry Pi OS with NetworkManager
- Chats and keys live in memory only and are lost when the client exits
- Flooding means every message is re-broadcast by every node – fine for a few nodes, not for large meshes
- Plain text messages are limited to ~200 characters in the TUI (and to 512 hex characters after encryption)
- Leftover files from earlier iterations are not part of the build: `tui/tui.cpp`, `tui/tui.h`, `udp-socket/src/node_udp.cpp`, `udp-socket/src/json__1_.cpp`, `udp-socket/include/mesh_node.h`

## License

Educational group project (PRJ3).
