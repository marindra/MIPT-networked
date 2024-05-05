#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <map>
//#include <Windows.h>

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;
std::map<uint16_t, unsigned int> scoresMap;

static uint16_t create_random_entity()
{
  uint16_t newEid = entities.size();
  uint32_t color = 0x44000000 * (1 + rand() % 4) +
                   0x00440000 * (1 + rand() % 4) +
                   0x00004400 * (1 + rand() % 4) +
                   0x0000003f * (2 + rand() % 3);
  float x = (rand() % 40 - 20) * 15.f;
  float y = (rand() % 40 - 20) * 15.f;
  Entity ent = {color, x, y, newEid, false, 0.f, 0.f, (size_t) (10  + rand() % 11)}; // start size is from 10 to 20
  entities.push_back(ent);
  scoresMap[newEid] = 0;
  return newEid;
}

void reset_size(Entity& ent)
{
  ent.sizeOfRect = ent.serverControlled ? 9 + rand() % 3 : 12; // [9; 11] <- for server controlled; 12 for controlled by player
}

int find_eaten(int i, int j, bool& needToEat)
{
  needToEat = false;
  Entity& first = entities[i];
  Entity& second = entities[j];

  //if ((2 * std::abs(first.x - second.x) < first.sizeOfRect + second.sizeOfRect) &&\
  //    (2 * std::abs(first.y - second.y) < first.sizeOfRect + second.sizeOfRect))
  if ((std::max(first.x, second.x) < std::min(first.x + first.sizeOfRect, second.x + second.sizeOfRect)) &&\
      (std::max(first.y, second.y) < std::min(first.y + first.sizeOfRect, second.y + second.sizeOfRect)))
  {
    // collision exists
    if (first.sizeOfRect == second.sizeOfRect)
    {
      return  i; // just to return. It may be a kind of trash.
    }

    needToEat = true;
    return first.sizeOfRect < second.sizeOfRect ? i : j;
  }

  return i; // just to return. It may be a kind of trash.
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t newEid = create_random_entity();
  const Entity& ent = entities[newEid];

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; size_t diameter = 10; size_t score = 0;
  deserialize_entity_state(packet, eid, x, y, diameter, score);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
      // e.sizeOfRect = diameter; // All size changes occur on the server...
      // e.score = score; // All score changes occur on the server...
    }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  constexpr int numAi = 10;

  for (int i = 0; i < numAi; ++i)
  {
    uint16_t eid = create_random_entity();
    entities[eid].serverControlled = true;
    controlledMap[eid] = nullptr;
  }

  uint32_t lastTime = enet_time_get();
  while (true)
  {
    uint32_t curTime = enet_time_get();
    float dt = (curTime - lastTime) * 0.001f;
    lastTime = curTime;
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    for (Entity &e : entities)
    {
      if (e.serverControlled)
      {
        const float diffX = e.targetX - e.x;
        const float diffY = e.targetY - e.y;
        const float dirX = diffX > 0.f ? 1.f : -1.f;
        const float dirY = diffY > 0.f ? 1.f : -1.f;
        constexpr float spd = 50.f;
        e.x += dirX * spd * dt;
        e.y += dirY * spd * dt;
        if (fabsf(diffX) < 10.f && fabsf(diffY) < 10.f)
        {
          e.targetX = (rand() % 40 - 20) * 15.f;
          e.targetY = (rand() % 40 - 20) * 15.f;
        }
      }
    }

    for (int i = 0; i + 1 < entities.size(); ++i)
    {
      for (int j = i + 1; j < entities.size(); ++j)
      {
        bool needToEat = false;
        int possibleEatenInd = find_eaten(i, j, needToEat);
        if (needToEat)
        {
          int eaterInd = i == possibleEatenInd ? j : i;
          Entity& eaten = entities[possibleEatenInd];
          Entity& eater = entities[eaterInd];
          //printf("%u - %zd (%f; %f) ate %u - %zd (%f; %f)\n", entities[eaterInd].eid, entities[eaterInd].sizeOfRect,\
          //                entities[eaterInd].x, entities[eaterInd].y, eaten.eid, eaten.sizeOfRect, eaten.x, eaten.y);

          eater.sizeOfRect += (eaten.sizeOfRect / 2);
          eater.score += (eaten.sizeOfRect / 2);

          if (eater.sizeOfRect >= 500)
          {
            eater.sizeOfRect = 500;
          }

          eaten.x = (rand() % 40 - 20) * 15.f;
          eaten.y = (rand() % 40 - 20) * 15.f;
          reset_size(eaten);
        }
      }
    }

    for (const Entity &e : entities)
    {
      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        //if (controlledMap[e.eid] != peer)
        send_snapshot(peer, e.eid, e.x, e.y, e.sizeOfRect, e.score);
      }
    }
    // Sleep(400);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


