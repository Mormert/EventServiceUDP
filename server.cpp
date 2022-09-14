#include "common.h"

#include <enet/enet.h>

#include <iostream>
#include <string>
#include <set>
#include <unordered_map>

void RunServerLoop(ENetHost *server) {
    static int peerId = 0;

    static std::unordered_map<std::string, std::set<ENetPeer *>> topicsWithSubscribedPeers;

    ENetEvent event;
    std::stringstream SS;
    std::string S;
    std::vector<std::string> recvDataDecode;
    char *d;

    /* Wait up to 100 milliseconds for an event. */
    while (enet_host_service(server, &event, 100) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n",
                       event.peer->address.host,
                       event.peer->address.port);
                /* We store the user id here */
                SS << "User_" << peerId++;
                S = SS.str();
                d = static_cast<char *>(malloc(S.size() + 1));
                strcpy_s(d, S.size() + 1, S.c_str());
                event.peer->data = d;
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                printf("A packet of length %zu containing `%s` was received from %s on channel %u.\n",
                       event.packet->dataLength,
                       event.packet->data,
                       event.peer->data,
                       event.channelID);

                recvDataDecode = split((char *) event.packet->data, ' ');

                if (recvDataDecode.size() == 2 && recvDataDecode.at(0) == "SUBSCRIBE") {
                    auto &set = topicsWithSubscribedPeers[recvDataDecode[1]];
                    set.insert(event.peer);
                }

                if (recvDataDecode.size() == 3 && recvDataDecode.at(0) == "PUBLISH") {
                    if (topicsWithSubscribedPeers.contains(recvDataDecode[1])) {
                        for (auto peer: topicsWithSubscribedPeers.at(recvDataDecode[1])) {
                            if (peer->state == ENET_PEER_STATE_CONNECTED) {
                                ENetPacket *packet = enet_packet_create(
                                        ("NEWS " + recvDataDecode[1] + ' ' + recvDataDecode[2]).c_str(),
                                        strlen(("NEWS " + recvDataDecode[1] + ' ' + recvDataDecode[2]).c_str()) + 1,
                                        ENET_PACKET_FLAG_RELIABLE);

                                enet_peer_send(peer, 0, packet);
                            }
                        }
                    }
                }

                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("%s disconnected.\n", event.peer->data);
                /* Reset the peer's client information. */
                free(event.peer->data);
                event.peer->data = nullptr;
                break;
            case ENET_EVENT_TYPE_NONE:
                break;
        }
        std::cout << std::flush;
    }
}

int main(int argc, char **argv) {
    if (enet_initialize() != 0) {
        std::cout << "ENet init failure occurred!" << std::endl;
        return EXIT_FAILURE;
    }

    ENetAddress address;
    ENetHost *server;
    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */
    address.host = ENET_HOST_ANY;
    /* Bind the server to port 1234. */
    address.port = 1234;
    server = enet_host_create(&address  /* the address to bind the server host to */,
                              32        /* allow up to 32 clients and/or outgoing connections */,
                              2         /* allow up to 2 channels to be used, 0 and 1 */,
                              0         /* assume any amount of incoming bandwidth */,
                              0         /* assume any amount of outgoing bandwidth */);
    if (server == nullptr) {
        fprintf(stderr,
                "An error occurred while trying to create an ENet server host.\n");
        exit(EXIT_FAILURE);
    }

    while (true) {
        RunServerLoop(server);
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
}