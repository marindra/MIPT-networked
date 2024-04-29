#include "raylib.h"
#include <enet/enet.h>
#include <iostream>
#include <string>
#include <map>

void send_reliable(ENetPeer *peer, const char *msg)
{
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_unsequenced(ENetPeer *peer, const char *msg)
{
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

int main(int argc, const char **argv)
{
  int width = 800;
  int height = 600;
  InitWindow(width, height, "Homework 1");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 3, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress gameAddress;
  ENetPeer *gamePeer;

  ENetAddress lobbyAddress;
  enet_address_set_host(&lobbyAddress, "localhost");
  lobbyAddress.port = 10887;

  ENetPeer *lobbyPeer = enet_host_connect(client, &lobbyAddress, 1, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }

  uint32_t timeStart = enet_time_get();
  uint32_t lastStartSendTime = timeStart;
  uint32_t lastMicroSendTime = timeStart;
  bool connectedToLobby = false;
  bool connectedToGame = false;
  bool gameStarted = false;
  bool connected = false;
  float posx = 0.f;
  float posy = 0.f;
  int ID = -1;
  std::string name = "";
  std::string status = "unknown";

  std::map<int, std::string> players;
  std::map<int, uint32_t> rttOfPlayers;

  while (!WindowShouldClose())
  {
    const float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        if (event.peer->address.host == lobbyAddress.host && event.peer->address.port == lobbyAddress.port) {
          connectedToLobby = true;
          status = "Connected to lobby.";
        }
        break;
      case ENET_EVENT_TYPE_RECEIVE:
      {
      // Possible messages:
      // Game server info. Host:localhost.Port:<port>
      // Current player info. ID:<ID>. Name:<name>.
      // New player info. ID:<ID>. Name:<name>.
      // Old player info. ID:<ID>. Name:<name>.
      // Ping info. ID:<ID>. Ping:<round trip time value>.
        printf("Packet received '%s'\n", event.packet->data);
        std::string recievedMessage = (const char *)event.packet->data;

        if (strncmp((const char *) event.packet->data, "Game server info.", 17) == 0) {
          if (strncmp((const char *) event.packet->data + 18, "Host:localhost.", 15) == 0) {
            enet_address_set_host(&gameAddress, "localhost");
            sscanf((const char *)event.packet->data, "Game server info. Host:localhost.Port:%u", &gameAddress.port);
          } else {
            printf("I don't know how to parse it! Use localhost instead or make some fixes.\n");
            enet_packet_destroy(event.packet);
            break;
          }
          //gamePeer

          gamePeer = enet_host_connect(client, &gameAddress, 2, 0);
          if (!gamePeer)
          {
            printf("Cannot connect to game server.\n");
            connectedToGame = false;
            printf("%x:%u\n", gameAddress.host, gameAddress.port);
            // Be Careful here!
          } else {
            printf("Connection with game server %x:%u established.\n", gamePeer->address.host, gamePeer->address.port);
            connectedToGame = true;
            status = "Game started.";
          }
        } else if (recievedMessage.find("player info.") != std::string::npos)
        {
          size_t pos = recievedMessage.find("ID:");
          if (pos == std::string::npos)
          {
            printf("Wrong player info structure. ID field is missing.\n");
            enet_packet_destroy(event.packet);
            break;
          }
          size_t posOfName = recievedMessage.find(". Name:");
          if (posOfName == std::string::npos)
          {
            printf("Wrong player info structure. Name field is missing.\n");
          }
          int IDfield = std::stoi(recievedMessage.substr(pos+3, posOfName - pos - 3));
          std::string nameField = recievedMessage.substr(posOfName + 7);

          printf("Get ID: %d.\nGet name: %s.\n", IDfield, nameField.c_str());

          if (strncmp((const char *) event.packet->data, "Current", 7) == 0)
          {
            status += " Name: ";
            status += nameField;
            name = nameField;
            ID = IDfield;
            players[ID] = name;
          }
          else if (strncmp((const char *) event.packet->data, "New", 3) == 0)
          {
            printf("Saved name '%s' of player with ID %d\n", nameField.c_str(), IDfield);
            players[IDfield] = nameField;
          }
          else if (strncmp((const char *) event.packet->data, "Old", 3) == 0)
          {
            auto iter = players.find(IDfield);
            if (iter == players.end())
            {
              printf("Saved name '%s' of player with ID %d\n", nameField.c_str(), IDfield);
              players[IDfield] = nameField;
            }
            else
            {
              if (iter->second != nameField)
              {
                printf("Something happen.\nI remember name '%s', but get other version ('%s'). Saved NEW name of player with ID %d\n", iter->second.c_str(), nameField.c_str(), IDfield);
               iter->second = nameField;
              }
            }
          }
          else
          {
            printf("Wrong player info structure. I don\'t know how to parse it.\n");
          }
        }

        enet_packet_destroy(event.packet);
        break;
      }
      default:
        break;
      };
    }
    if (connectedToGame)
    {
      //std::string message = std::string("Position info. ID:") + 
      //send_unsequenced(gamePeer, message.c_str());
    }
    if (connectedToLobby && !connectedToGame && IsKeyDown(KEY_ENTER) && enet_time_get() - lastStartSendTime >= 100)
    {
      // Cooldown time = 10 is too small so I set value=100
      printf("Sending start message to lobby.\n");
      send_reliable(lobbyPeer, "Game started.");
      lastStartSendTime = enet_time_get();
    }

    bool left = IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_RIGHT);
    bool up = IsKeyDown(KEY_UP);
    bool down = IsKeyDown(KEY_DOWN);
    constexpr float spd = 10.f;
    posx += ((left ? -1.f : 0.f) + (right ? 1.f : 0.f)) * dt * spd;
    posy += ((up ? -1.f : 0.f) + (down ? 1.f : 0.f)) * dt * spd;

    BeginDrawing();
      ClearBackground(BLACK);
      DrawText(TextFormat("Current status: %s", status.c_str()), 20, 20, 20, WHITE);
      DrawText(TextFormat("My position: (%d, %d)", (int)posx, (int)posy), 20, 40, 20, WHITE);
      DrawText("List of players:", 20, 60, 20, WHITE);
      int counter = 80;
      for (const auto& elem : players)
      {
        DrawText(TextFormat("%s", elem.second.c_str()), 20, counter, 20, WHITE);
        counter += 20;
      }
    EndDrawing();
  }
  return 0;
}
