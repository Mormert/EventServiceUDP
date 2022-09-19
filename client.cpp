#include "common.h"

#include <enet/enet.h>

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std::chrono_literals;

int main(int argc, char **argv) {

    std::string publisherOrSubsriberStr;
    while (!(publisherOrSubsriberStr == "P" || publisherOrSubsriberStr == "S")) {
        std::cout << "Are you a publisher (P) or subscriber (S)? ";
        std::getline(std::cin, publisherOrSubsriberStr);
        std::transform(publisherOrSubsriberStr.begin(), publisherOrSubsriberStr.end(), publisherOrSubsriberStr.begin(),
                       ::toupper);
    }

    bool isSubscriber = false;
    if (publisherOrSubsriberStr == "S") {
        isSubscriber = true;
    }

    if (enet_initialize() != 0) {
        std::cout << "ENet init failure occurred!" << std::endl;
        return EXIT_FAILURE;
    }

    ENetHost *client;
    std::mutex NetMtx;
    client = enet_host_create(nullptr /* create a client host */,
                              1 /* only allow 1 outgoing connection */,
                              2 /* allow up 2 channels to be used, 0 and 1 */,
                              0 /* assume any amount of incoming bandwidth */,
                              0 /* assume any amount of outgoing bandwidth */);
    if (client == nullptr) {
        fprintf(stderr,
                "An error occurred while trying to create an ENet client host.\n");
        exit(EXIT_FAILURE);
    }


    ENetAddress address;
    ENetEvent event;
    ENetPeer *peer;
    /* Connect to some.server.net:1234. */
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 1234;
    /* Initiate the connection, allocating the two channels 0 and 1. */
    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == nullptr) {
        fprintf(stderr,
                "No available peers for initiating an ENet connection.\n");
        exit(EXIT_FAILURE);
    }

    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        puts("Connection to 127.0.0.1:1234 succeeded.");

    } else {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);
        puts("Connection to 127.0.0.1:1234 failed.");
        int a;
        std::cin >> a;
        std::exit(1);
    }


    if (isSubscriber) {
        auto userInput = std::thread{[&]() {
            while (true) {

                std::string subOrUnsub;
                while (!(subOrUnsub == "S" || subOrUnsub == "U")) {
                    std::cout << "Do you want to subscribe (S) or unsubscribe (U)? ";
                    std::getline(std::cin, subOrUnsub);
                    std::transform(subOrUnsub.begin(), subOrUnsub.end(), subOrUnsub.begin(), ::toupper);
                }

                if (subOrUnsub == "S") {
                    std::cout << "Enter what you want to subscribe to: ";
                    std::string subTo;
                    getline(std::cin, subTo);
                    if (subTo.empty()) {
                        continue;
                    }
                    std::transform(subTo.begin(), subTo.end(), subTo.begin(), ::toupper);

                    std::lock_guard<std::mutex> lockGuard{NetMtx};

                    std::string packetStr = "SUBSCRIBE " + subTo;
                    ENetPacket *packet = enet_packet_create(packetStr.c_str(),
                                                            strlen(packetStr.c_str()) + 1,
                                                            ENET_PACKET_FLAG_RELIABLE);

                    enet_peer_send(peer, 0, packet);
                } else {
                    std::cout << "Enter what you want to unsubscribe to: ";
                    std::string unsubTo;
                    getline(std::cin, unsubTo);
                    if (unsubTo.empty()) {
                        continue;
                    }
                    std::transform(unsubTo.begin(), unsubTo.end(), unsubTo.begin(), ::toupper);

                    std::lock_guard<std::mutex> lockGuard{NetMtx};

                    std::string packetStr = "UNSUBSCRIBE " + unsubTo;
                    ENetPacket *packet = enet_packet_create(packetStr.c_str(),
                                                            strlen(packetStr.c_str()) + 1,
                                                            ENET_PACKET_FLAG_RELIABLE);

                    enet_peer_send(peer, 0, packet);
                }


            }
        }};

        while (true) {

            std::this_thread::sleep_for(25ms);

            std::lock_guard<std::mutex> lockGuard{NetMtx};

            while (enet_host_service(client, &event, 100) > 0) {
                if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                    printf("A packet of length %zu containing `%s` was received from %s on channel %u.\n",
                           event.packet->dataLength,
                           event.packet->data,
                           event.peer->data,
                           event.channelID);

                    const auto recvDataDecode = split((char *) event.packet->data, ' ');

                    if (recvDataDecode.size() == 3 && recvDataDecode.at(0) == "NEWS") {
                        const auto &category = recvDataDecode[1];
                        const auto &message = recvDataDecode[2];
                        std::cout << "News incoming in category " << category << ": " << message << std::endl;
                    }
                }
            }
        }
    } else {

        // If we're a publisher

        auto enetHostService = std::thread{[&]() {
            while (true) {
                std::this_thread::sleep_for(25ms);

                std::lock_guard<std::mutex> lockGuard{NetMtx};

                enet_host_service(client, &event, 0);
            }
        }};

        while (true) {
            std::cout << "Enter the category of news you want to report: ";
            std::string category;
            getline(std::cin, category);
            std::transform(category.begin(), category.end(), category.begin(), ::toupper);

            std::cout << "Enter what happened: ";
            std::string message;
            getline(std::cin, message);


            std::lock_guard<std::mutex> lockGuard{NetMtx};

            std::string packetStr = "PUBLISH " + category + " " + message;
            ENetPacket *packet = enet_packet_create(packetStr.c_str(),
                                                    strlen(packetStr.c_str()) + 1,
                                                    ENET_PACKET_FLAG_RELIABLE);

            enet_peer_send(peer, 0, packet);

            enet_host_service(client, &event, 0);
        }

    }

}

/*enet_peer_disconnect (peer, 0);

while (enet_host_service (client, & event, 3000) > 0)
{
    switch (event.type)
    {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy (event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            std::cout << "Disconnection succeeded." << std::endl;
            break;
    }
}

enet_host_destroy(client);

atexit(enet_deinitialize);
*/