#!/bin/bash

echo "==============================="
echo "  Batman-adv Mesh Cleanup"
echo "==============================="

# Stop and disable service
echo ">>> Stopping and disabling batman-mesh service..."
sudo systemctl stop batman-mesh
sudo systemctl disable batman-mesh

# Delete service file
echo ">>> Deleting service file..."
sudo rm -f /etc/systemd/system/batman-mesh.service

# Delete mesh folder
echo ">>> Deleting mesh folder..."
rm -rf ~/mesh

# Remove batman-adv from /etc/modules
echo ">>> Removing batman-adv from /etc/modules..."
sudo sed -i '/batman-adv/d' /etc/modules

# Remove NetworkManager unmanaged config
echo ">>> Removing NetworkManager config..."
sudo rm -f /etc/NetworkManager/conf.d/unmanaged-wlan0.conf

# Let NetworkManager manage wlan0 again
echo ">>> Giving wlan0 back to NetworkManager..."
sudo nmcli device set wlan0 managed yes

# Reload systemd
sudo systemctl daemon-reload

# Restart NetworkManager
sudo systemctl restart NetworkManager

echo ""
echo "==============================="
echo "  Cleanup complete!"
echo "==============================="
echo ">>> Verify with:"
echo "    nmcli device status    — wlan0 should show managed"
echo "    iwconfig wlan0         — should show Mode:Managed"
