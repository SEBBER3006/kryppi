#include "run_tui.h"
#include "../../udp-socket/inc/udp_socket.hpp"
#include "../../udp-socket/inc/config.hpp"
#include <thread>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>

int main(int argc, char* argv[]) {
     if (argc > 1) {                                                                                                            
       // Start without networking if no IP given (useful for UI testing)                                                     
        Run_TUI tui;                                                                                                           
        tui.run();                                                                                                             
        return 0;                                                                                                              
     }            

    // Læs IP: fra argument eller fra fil
    std::string myIp;
     myIp = readIpFromFile();                                                                                                   
    if (myIp.empty()) {                                                                                                        
    std::cerr << "Fejl: Ingen IP angivet og "                                                                              
    << NODE_IP_FILE << " ikke fundet.\n"                                                                           
    << "Brug: " << argv[0] << " <ip>  eller kør setup-mesh.sh først." << std::endl;                                                                                                  
      return EXIT_FAILURE;  
    }

    srand(time(NULL));                                                                                                         
    MeshNode node(myIp);      
    Run_TUI tui;
    tui.set_my_ip(myIp);

    // Incoming: network → TUI
    node.on_receive = [&tui](const std::string& from, const std::string& text) -> bool {
        return tui.receive_message(from, text);
    };

    // ACK: network → TUI
    node.on_ack = [&tui](const std::string& from, const std::string& type, uint32_t msgId) {
        tui.receive_ack(from, type, msgId);
    };

    // Outgoing: TUI → network
    tui.set_send_callback([&node](const std::string& to, const std::string& text, const std::string& key) -> uint32_t {
        return node.createAndSend(to, text, key);
    });

    std::thread netThread(&MeshNode::receiveAndSend, &node);
    netThread.detach();

    tui.run();  
    return 0;
}
