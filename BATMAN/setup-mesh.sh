#!/bin/bash
set -e

# ── 1. Derive IP from MAC ──────────────────────────────────────────────
MAC=$(cat /sys/class/net/wlan0/address)
O5=$(printf '%d' 0x$(echo $MAC | cut -d: -f5))
O6=$(printf '%d' 0x$(echo $MAC | cut -d: -f6))

[ $O5 -eq 0 ]   && O5=1
[ $O5 -eq 255 ] && O5=254
[ $O6 -eq 0 ]   && O6=1
[ $O6 -eq 255 ] && O6=254

NODE_IP="10.0.$O5.$O6"
echo ">>> MAC: $MAC  →  IP: $NODE_IP"
echo ">>> Auto-assigned IP: $NODE_IP"

# Step 1 - Install batctl
echo ">>> Installing batctl..."
sudo apt-get install -y batctl

# Step 2 - Create mesh folder
echo ">>> Creating mesh folder..."
mkdir -p ~/mesh
cp -r ./* ~/mesh/

# Step 3 - Create startup script
echo ">>> Creating startup script..."
cat > ~/mesh/start-batman-adv.sh << EOF
#!/bin/bash
sudo ip addr flush dev bat0 2>/dev/null
sudo modprobe batman-adv
sudo ip link set wlan0 down
sudo iw dev wlan0 set type ibss
sudo ip link set wlan0 up
sudo iw dev wlan0 ibss join my-mesh 2412
sudo batctl if add wlan0
sudo ip link set dev bat0 mtu 1468
sudo batctl gw_mode client
sudo ip link set up dev bat0
sudo ip addr add $NODE_IP/16 dev bat0
EOF

# Make it executable
chmod +x ~/mesh/start-batman-adv.sh

# Step 4 - Load batman-adv at boot
echo ">>> Adding batman-adv to /etc/modules..."
echo 'batman-adv' | sudo tee --append /etc/modules

# Step 5 - Stop NetworkManager managing wlan0
echo ">>> Disabling NetworkManager for wlan0..."
sudo nmcli device set wlan0 managed no

# Step 5b - Make wlan0 unmanaged permanent
echo ">>> Making wlan0 permanently unmanaged..."
sudo bash -c 'cat > /etc/NetworkManager/conf.d/unmanaged-wlan0.conf << EOF
[keyfile]
unmanaged-devices=interface-name:wlan0
EOF'
sudo systemctl restart NetworkManager

# Step 6 - Create systemd service
echo ">>> Creating systemd service..."
USER_HOME="/home/$USER"
sudo bash -c "cat > /etc/systemd/system/batman-mesh.service << 'EOF'
[Unit]
Description=Batman-adv Mesh Network
After=network.target NetworkManager.service
Wants=NetworkManager.service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=${USER_HOME}/mesh/start-batman-adv.sh

[Install]
WantedBy=multi-user.target
EOF"

# Enable service
sudo systemctl daemon-reload
sudo systemctl enable batman-mesh

# Send IP to TUI, save in txt file
echo $NODE_IP > ~/mesh/node_ip.txt
echo ">>> Node IP $NODE_IP saved to ~/mesh/node_ip.txt for TUI use."

echo ">>> Setup complete! Rebooting in 5 seconds..."
sleep 5
sudo reboot