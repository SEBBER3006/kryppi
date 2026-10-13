#pragma once
 
#include <string>
 
//Netværkskonfiguration
#define PORT        8888
#define BUFFER_SIZE 1024
 
// Broadcast-adresse
#define BROADCAST_ADDR "10.0.255.255"
 
// Fil gemt af setup-mesh.sh 
#define NODE_IP_FILE "/home/pi/mesh/node_ip.txt"
 
// Krypteringsnøgle
const std::string CRYPTO_KEY = "BlueEyesMotherFucker";