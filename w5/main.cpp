// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <vector>
#include <map>
#include <deque>
#include "entity.h"
#include "protocol.h"

static uint32_t lastVerifiesTicknum = 0;
static uint32_t myInd = 0;
static std::deque<Entity> myOldStates;
static std::vector<Entity> entities;
static std::map<uint32_t, std::deque<Entity>> history_of_entities; // it's for future snapshots
static uint16_t my_entity = invalid_entity;
const uint32_t fixedDt = 20;
const float eps = 0.001f;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity; uint32_t timestamp = 0;
  deserialize_new_entity(packet, newEntity);
  // TODO: Direct adressing, of course!
  if (history_of_entities.find(newEntity.eid) != history_of_entities.end())
  {
    return; // don't need to do anything, we already have entity
  }
  for (const Entity &e : entities)
    if (e.eid == newEntity.eid)
      return; // don't need to do anything, we already have entity
  //newEntity.timestamp *= fixedDt;
  entities.push_back(newEntity);
  newEntity.timestamp /= fixedDt;
  history_of_entities[newEntity.eid] = std::deque<Entity>(1, newEntity);
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
  for (int i = 0; i < entities.size(); ++i)
    if (entities[i].eid == my_entity)
      myInd = i;
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float ori = 0.f; uint32_t timestamp = 0;
  deserialize_snapshot(packet, eid, x, y, ori, timestamp);
  // TODO: Direct adressing, of course!
  //for (Entity &e : entities)
  //  if (e.eid == eid)
  //  {
  //    e.x = x;
  //    e.y = y;
  //    e.ori = ori;
  //  }
  
  auto dequeForEntity = history_of_entities.find(eid);
  if (dequeForEntity != history_of_entities.end())
  {
    uint32_t recievedTickNum = timestamp / fixedDt;
    Entity& newestEntity = dequeForEntity->second.back();
    if (recievedTickNum > newestEntity.timestamp)
    {
      dequeForEntity->second.emplace_back(newestEntity.color, x, y, newestEntity.speed, ori,
        newestEntity.thr, newestEntity.steer, eid, timestamp / fixedDt);
    } else {
      // need to check my state
      if (recievedTickNum < lastVerifiesTicknum || eid != my_entity || myOldStates.size() == 0)
      {
        return;
      }

      int32_t shift = recievedTickNum - (myOldStates.front().timestamp / fixedDt);
      if (shift < 0 || shift >= myOldStates.size())
      {
        // I can't do something - it's too old. I can't save so big history
        // the second condition - just for safety
        return;
      }

      lastVerifiesTicknum = recievedTickNum;
      Entity& myStateAtThisMoment = myOldStates[shift];
      if (fabs(x - myStateAtThisMoment.x) <= eps && fabs(y - myStateAtThisMoment.y) <= eps && fabs(ori - myStateAtThisMoment.ori) <= eps)
      {
        // It's verified state now
        myOldStates.erase(myOldStates.begin(), myOldStates.begin() + std::min((int) myOldStates.size() - 1, shift));
        return;
      }

      // Here I must to regenerate information about my entity
      myStateAtThisMoment.x = x;
      myStateAtThisMoment.y = y;
      myStateAtThisMoment.ori = ori;

      // start to create new ones
      Entity tmpEnt = myOldStates[shift];
      for (int i = shift + 1; i < myOldStates.size(); ++i)
      {
        tmpEnt.thr = myOldStates[i].thr;
        tmpEnt.steer = myOldStates[i].steer;
        simulate_entity(tmpEnt, 0.001f * fixedDt);
        myOldStates[i].x = tmpEnt.x;
        myOldStates[i].y = tmpEnt.y;
        myOldStates[i].ori = tmpEnt.ori;
      }

      // And apply changes to current position:
      entities[myInd].x = tmpEnt.x;
      entities[myInd].y = tmpEnt.y;
      entities[myInd].ori = tmpEnt.ori;
    }
  }
}
//uint32_t lastPrint = 0;
void updateEntity(Entity& e)
{
  auto dequeForEntity = history_of_entities.find(e.eid);
  if (dequeForEntity == history_of_entities.end())
  {
    return;
  }
  if (dequeForEntity->second.size() == 0)
  {
    return;
  }

  /*{
    Entity& entity_to_copy = dequeForEntity->second.back();
    e.ori = entity_to_copy.ori;
    e.x = entity_to_copy.x;
    e.y = entity_to_copy.y;
    dequeForEntity->second.erase(dequeForEntity->second.begin(), dequeForEntity->second.begin() + dequeForEntity->second.size() - 1);
    return;
  }*/

  uint32_t curTimestamp = enet_time_get() - 400;//if (lastPrint + 200 < curTimestamp){
  //printf("%u\n", curTimestamp);lastPrint=curTimestamp;}
  uint32_t curTick = curTimestamp / fixedDt;
  for (int i = 0; i < dequeForEntity->second.size(); ++i)
  {
    if (dequeForEntity->second[i].timestamp > curTick)
    {
      // update (interpolate) and clean extra ones
      float scaler = 1.0f * (curTimestamp - e.timestamp) / \
          (dequeForEntity->second[i].timestamp * fixedDt - e.timestamp);
      e.ori = dequeForEntity->second[i].ori * scaler + e.ori * (1 - scaler);
      e.x = dequeForEntity->second[i].x * scaler + e.x * (1 - scaler);
      e.y = dequeForEntity->second[i].y * scaler + e.y * (1 - scaler);
      e.timestamp = curTimestamp;

      if (i != 0)
        dequeForEntity->second.erase(dequeForEntity->second.begin(), dequeForEntity->second.begin() + i);
      return;
    }
  }

  //need to copy first one and say about this situation
  //printf("Don\'t have information about future\n");
  //Entity& entity_to_copy = dequeForEntity->second.back();
  //e.ori = entity_to_copy.ori;
  //e.x = entity_to_copy.x;
  //e.y = entity_to_copy.y;
  //e.timestamp = curTimestamp;
  simulate_entity(e, 0.001f * fixedDt);
  return;
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;


  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  bool connected = false;
  uint32_t lastInputSend = 0;
  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_SET_TIME:
          {
          uint32_t timeToSet = 0;
          deserialize_time(event.packet, timeToSet);
          enet_time_set(timeToSet + event.peer->roundTripTime / 2);//lastPrint = timeToSet;
          //printf("\n\t\tRtt: %u\n\n", (uint32_t)event.peer->roundTripTime);
          }
          break;
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        };
        break;
      default:
        printf("Strange...\n");
        break;
      };
    }
    if (my_entity != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      if (enet_time_get() - lastInputSend >= fixedDt)
      {
        for (Entity &e : entities)
        {
          if (e.eid == my_entity)
          {
            // Update
            float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
            float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

            // Send
            send_entity_input(serverPeer, my_entity, thr, steer);
            e.thr = thr;
            e.steer = steer;
           //simulate_entity(e, dt);
            //e.timestamp += dt;
          }
          updateEntity(e);
          if (e.eid == my_entity)
          {
            myOldStates.push_back(e);
            if (myOldStates.size() >= 200)
            {
              myOldStates.erase(myOldStates.begin(), myOldStates.begin() + 100);
            }
          }

          lastInputSend = enet_time_get();
        }
      }
    }

    BeginDrawing();
      ClearBackground(GRAY);
      BeginMode2D(camera);
        for (Entity &e : entities)
        {
          const Rectangle rect = {e.x, e.y, 3.f, 1.f};
          DrawRectanglePro(rect, {0.f, 0.5f}, e.ori * 180.f / PI, GetColor(e.color));
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
