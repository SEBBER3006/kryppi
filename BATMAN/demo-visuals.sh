#!/bin/bash

# ─────────────────────────────────────────────
#  ANSI colours & styles
# ─────────────────────────────────────────────
RESET="\033[0m"
BOLD="\033[1m"
DIM="\033[2m"

BLACK="\033[30m"
RED="\033[31m"
GREEN="\033[32m"
YELLOW="\033[33m"
BLUE="\033[34m"
MAGENTA="\033[35m"
CYAN="\033[36m"
WHITE="\033[37m"

BG_BLACK="\033[40m"
BG_BLUE="\033[44m"
BG_CYAN="\033[46m"

# ─────────────────────────────────────────────
#  Helpers
# ─────────────────────────────────────────────

print_banner() {
    echo ""
    echo -e "${BOLD}${BG_BLUE}${WHITE}                                         ${RESET}"
    echo -e "${BOLD}${BG_BLUE}${WHITE}   🦇  Batman-adv Mesh Network Setup  🦇  ${RESET}"
    echo -e "${BOLD}${BG_BLUE}${WHITE}                                         ${RESET}"
    echo ""
}

print_step() {
    local num=$1
    local msg=$2
    echo -e "\n${BOLD}${CYAN}[${num}]${RESET} ${BOLD}${WHITE}${msg}${RESET}"
}

print_ok() {
    echo -e "    ${GREEN}✔${RESET}  $1"
}

print_info() {
    echo -e "    ${DIM}${WHITE}→  $1${RESET}"
}

print_warn() {
    echo -e "    ${YELLOW}⚠${RESET}  $1"
}

print_error() {
    echo -e "    ${RED}✘${RESET}  $1"
}

# Spinner — runs while a background PID is alive
spinner() {
    local pid=$1
    local label=$2
    local frames=("⠋" "⠙" "⠹" "⠸" "⠼" "⠴" "⠦" "⠧" "⠇" "⠏")
    local i=0
    tput civis  # hide cursor
    while kill -0 "$pid" 2>/dev/null; do
        printf "\r    ${CYAN}%s${RESET}  %s " "${frames[$((i % ${#frames[@]}))]}" "$label"
        sleep 0.1
        ((i++))
    done
    printf "\r    ${GREEN}✔${RESET}  %-40s\n" "$label"
    tput cnorm  # restore cursor
}

# Progress bar — fake timed bar, $1 = label, $2 = duration (seconds)
progress_bar() {
    local label=$1
    local duration=${2:-3}
    local width=35
    local interval
    interval=$(echo "scale=4; $duration / $width" | bc)

    echo -e "    ${DIM}${WHITE}${label}${RESET}"
    printf "    ["
    for ((i=0; i<width; i++)); do
        printf "${GREEN}█${RESET}"
        sleep "$interval"
    done
    printf "] ${GREEN}done${RESET}\n"
}

# Section divider
divider() {
    echo -e "\n    ${DIM}${WHITE}────────────────────────────────────────${RESET}"
}

# Final summary table
print_summary() {
    local node_ip=$1
    echo ""
    echo -e "${BOLD}${BG_CYAN}${BLACK}   Summary                                ${RESET}"
    echo ""
    printf "    %-22s ${GREEN}%s${RESET}\n" "Node IP:"        "$node_ip"
    printf "    %-22s ${GREEN}%s${RESET}\n" "Mesh SSID:"      "my-mesh"
    printf "    %-22s ${GREEN}%s${RESET}\n" "Channel:"        "1  (2412 MHz)"
    printf "    %-22s ${GREEN}%s${RESET}\n" "Interface:"      "bat0  →  wlan0"
    printf "    %-22s ${GREEN}%s${RESET}\n" "Service:"        "batman-mesh  (enabled)"
    printf "    %-22s ${GREEN}%s${RESET}\n" "IP saved to:"    "~/mesh/node_ip.txt"
    echo ""
}

# ─────────────────────────────────────────────
#  Demo  (mirrors setup-mesh.sh steps)
# ─────────────────────────────────────────────

print_banner

# ── Step 0: IP assignment ──────────────────────────────────
print_step "0" "Claiming node IP address"
print_info "Switching to node_ip branch and pulling..."
sleep 0.8
NODE_IP="10.0.0.3"   # hardcoded for demo
print_ok "Auto-assigned IP: ${BOLD}${YELLOW}${NODE_IP}${RESET}"
print_info "Appended to claimed_ips.txt and pushed to remote."

divider

# ── Step 1: Install batctl ─────────────────────────────────
print_step "1" "Installing batctl"
progress_bar "apt-get install -y batctl" 2.5
print_ok "batctl installed successfully."

divider

# ── Step 2: Create mesh folder ────────────────────────────
print_step "2" "Creating mesh folder"
(sleep 1.2) &
spinner $! "Copying files to ~/mesh/"
print_info "Destination: /home/pi/mesh/"

divider

# ── Step 3: Startup script ────────────────────────────────
print_step "3" "Writing start-batman-adv.sh"
(sleep 0.8) &
spinner $! "Generating startup script"
print_ok "Script written to ~/mesh/start-batman-adv.sh"
print_ok "chmod +x applied."

divider

# ── Step 4: /etc/modules ──────────────────────────────────
print_step "4" "Configuring kernel modules"
sleep 0.5
print_ok "batman-adv appended to /etc/modules"
print_info "Module will load automatically at boot."

divider

# ── Step 5: NetworkManager ────────────────────────────────
print_step "5" "Releasing wlan0 from NetworkManager"
(sleep 1.0) &
spinner $! "Setting wlan0 unmanaged"
print_ok "Wrote /etc/NetworkManager/conf.d/unmanaged-wlan0.conf"
(sleep 0.6) &
spinner $! "Restarting NetworkManager"
print_ok "NetworkManager restarted — wlan0 is now free."

divider

# ── Step 6: systemd service ───────────────────────────────
print_step "6" "Registering systemd service"
(sleep 0.5) &
spinner $! "Writing batman-mesh.service"
(sleep 0.4) &
spinner $! "systemctl daemon-reload"
(sleep 0.4) &
spinner $! "systemctl enable batman-mesh"
print_ok "Service enabled — mesh will start on every boot."

divider

# ── Summary ───────────────────────────────────────────────
print_summary "$NODE_IP"

echo -e "${BOLD}${YELLOW}  ⚡  Rebooting in 5 seconds...${RESET}"
echo -e "${DIM}     Reconnect via SSH after reboot and verify with:${RESET}"
echo -e "${DIM}       iwconfig wlan0   |   ip addr show bat0   |   sudo batctl n${RESET}"
echo ""

# Countdown
for i in 5 4 3 2 1; do
    printf "\r    ${BOLD}${RED}%d${RESET} " "$i"
    sleep 1
done
echo ""
echo -e "    ${DIM}(skipping actual reboot — this is a demo)${RESET}"
echo ""
