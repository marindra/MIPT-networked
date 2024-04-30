#include <enet/enet.h>
#include <iostream>

int main(int argc, const char **argv)
{
  printf("Version 1.07\n");

  const int maxPlayerCount = 32;
  bool gameStarted = false;

  const uint16_t gameServerPort = 10889;


  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress lobbyServerAddress;

  lobbyServerAddress.host = ENET_HOST_ANY;
  lobbyServerAddress.port = 10887;

  ENetHost *lobbyServer = enet_host_create(&lobbyServerAddress, maxPlayerCount, 2, 0, 0);

  if (!lobbyServer)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(lobbyServer, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        if (gameStarted)
        {
          char message[45];
          snprintf(message, 45, "Game server info. Host:localhost.Port:%u", gameServerPort);

          printf("Sending game server address to new client.\n");
          ENetPacket *packet = enet_packet_create(message, strlen(message) + 1, ENET_PACKET_FLAG_RELIABLE);
          enet_peer_send(event.peer, 0, packet);
        }

        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);

        if (!gameStarted && strcmp((const char *)event.packet->data, "Game started.") == 0)
        {
          printf("Game started!\n");
          gameStarted = true;
          // Need to send it to everyone

          char message[45];
          snprintf(message, 45, "Game server info. Host:localhost.Port:%u", gameServerPort);
          printf("Sending start message to every peer.\n");
          ENetPacket *packet = enet_packet_create(message, strlen(message) + 1, ENET_PACKET_FLAG_RELIABLE);
          enet_host_broadcast(lobbyServer, 0, packet);
        }

        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
  }

  enet_host_destroy(lobbyServer);

  atexit(enet_deinitialize);
  printf("Here!");
  return 0;
}

