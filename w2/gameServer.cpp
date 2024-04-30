#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <string>

struct Player
{
    std::string name;
    int ID = -1;
    ENetPeer *peer;
};

int main(int argc, const char **argv)
{
  const int maxPlayerCount = 32;
  int IDcounter = 0;
  std::vector<std::string> names = {"Kate", "Alex", "Den", "Kris", "Kirill", "Pavel", "Ann", "Anna", "Sanya", "Jasmine",\
  "Penelopa", "Aaron", "Emma", "Emily", "Jacob", "Daniel", "Damies", "Michael", "Evelyn", "Mike", "William", "Adam",\
  "Kevin", "David", "Kellie", "Ryan", "Jordan", "Dylan", "John", "Jack", "Blaire", "Tyler"};
  std::vector<Player> connectedPlayers;

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10889;

  ENetHost *server = enet_host_create(&address, maxPlayerCount, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  printf("Game server started.\n"); 
  uint32_t lastPing = enet_time_get();

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
      {
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        connectedPlayers.emplace_back(names[IDcounter % maxPlayerCount], IDcounter, event.peer);

        std::string newPlayerInfo = "Current player info. ID:" + std::to_string(IDcounter) + ". Name:" + names[IDcounter % maxPlayerCount];
        printf("Send information '%s' about player to this player\n\t(through peer with pointer '%p')\n", newPlayerInfo.c_str(), connectedPlayers[IDcounter].peer);
        ENetPacket *newPacket = enet_packet_create(newPlayerInfo.c_str(), newPlayerInfo.size() + 1, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(connectedPlayers[IDcounter].peer, 0, newPacket);

        printf("Send information about new player to players.\n");
        newPlayerInfo = "New player info. ID:" + std::to_string(IDcounter) + ". Name:" + names[IDcounter % maxPlayerCount];
        newPacket = enet_packet_create(newPlayerInfo.c_str(), newPlayerInfo.size() + 1, ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(server, 0, newPacket);

        std::string oldPlayerInfo;
        // send information about old players
        printf("Start reliable sending information about old users.\n");
        for (int i = 0; i < connectedPlayers.size() - 1; ++i) {
          oldPlayerInfo = std::string("Old player info. ID:") + std::to_string(connectedPlayers[i].ID) + std::string(". Name:") + connectedPlayers[i].name;
          printf("Send '%s' to player with ID %d (reliable)\n\t(through peer with pointer '%p')\n", oldPlayerInfo.c_str(), IDcounter, event.peer);
          ENetPacket *packet = enet_packet_create(oldPlayerInfo.c_str(), oldPlayerInfo.size() + 1, ENET_PACKET_FLAG_RELIABLE);
          //enet_host_broadcast(server, 0, packet);
          enet_peer_send(connectedPlayers[IDcounter].peer, 0, packet);
        }
        // printf("\tEvent peer pointer is %p\n", event.peer);

        IDcounter += 1;
        break;
      }
      case ENET_EVENT_TYPE_RECEIVE:
        //if (strncmp((const char *) event.packet->data, "Position info.", 14) != 0)
        //{
        printf("Packet received '%s'\n", event.packet->data);
        //}
        enet_packet_destroy(event.packet);
        // I don't see task to show positions of all players, so I don't work with recieved position
        break;
      default:
        break;
      };
      
    }

    std::string pingInfo;
    ENetPacket *packet;
    if (enet_time_get() - lastPing >= 500)
    {
      printf("Send ping info about all users. (unreliable.)\n");
      for (int i = 0; i < connectedPlayers.size(); ++i)
      {
        pingInfo = std::string("Ping info. ID:") + std::to_string(connectedPlayers[i].ID) + std::string(". Rtt:") + std::to_string(connectedPlayers[i].peer->roundTripTime);
        packet = enet_packet_create(pingInfo.c_str(), pingInfo.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
        enet_host_broadcast(server, 1, packet);
      }
      lastPing = enet_time_get();
    }
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}