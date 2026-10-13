# Batman-adv Mesh Network — Setup Guide

This guide explains how to connect a Raspberry Pi to the batman-adv mesh network and how to fully disconnect it when needed.

---

## Connect / Setup

Run these steps on **each Raspberry Pi** you want to add to the mesh.

### 1. Navigate to the BATMAN folder

```bash
cd prj3-10/BATMAN
```

### 2. Make the setup script executable

```bash
chmod +x setup-mesh.sh
```

### 3. Run the setup script

```bash
./setup-mesh.sh
```

The script will automatically:

- **Claim a unique IP address** for this node (e.g. `10.0.0.1`, `10.0.0.2`, …) by reading and updating `claimed_ips.txt` in the `node_ip` git branch.
- **Install `batctl`**, the batman-adv control tool.
- **Create `~/mesh/`** with all required scripts and save the node's IP to `~/mesh/node_ip.txt`.
- **Configure `wlan0`** in IBSS (ad-hoc) mode and join the mesh SSID `my-mesh` on channel 1 (2412 MHz).
- **Take `wlan0` out of NetworkManager** so it is permanently managed by batman-adv instead.
- **Register a systemd service** (`batman-mesh`) that starts the mesh automatically on every boot.
- **Reboot automatically** after 5 seconds to apply all changes.

### 4. Verify the connection

After the RPi has rebooted and you have reconnected via SSH, confirm everything is working:

```bash
iwconfig wlan0          # Mode should be Ad-Hoc, ESSID: my-mesh
ip addr show bat0       # Should show 10.0.0.x/24
sudo batctl if          # Should list wlan0 as active
sudo batctl n           # Should show neighbouring nodes when more RPis are running
```

---

## Disconnect

To fully remove the mesh setup from a Raspberry Pi and return it to normal Wi-Fi operation, run the cleanup script.

### 1. Make the cleanup script executable

```bash
chmod +x cleanup-mesh.sh
```

### 2. Run the cleanup script

```bash
./cleanup-mesh.sh
```

The script will automatically:

- **Stop and disable** the `batman-mesh` systemd service.
- **Delete** the service file and the `~/mesh/` folder.
- **Remove `batman-adv`** from `/etc/modules` so it no longer loads at boot.
- **Remove the NetworkManager override** that was blocking `wlan0`.
- **Hand `wlan0` back to NetworkManager** so normal Wi-Fi resumes.

### 3. Verify the cleanup

```bash
nmcli device status     # wlan0 should show as managed
iwconfig wlan0          # Mode should be Managed (normal Wi-Fi)
```
