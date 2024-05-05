#include "protocol.h"
#include "bitstream.h"
#include <assert.h>

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  //*packet->data = E_CLIENT_TO_SERVER_JOIN;
  Bitstream bs(packet);
  bs.write(E_CLIENT_TO_SERVER_JOIN);
  assert(bs.checkIsEnd());

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  //uint8_t *ptr = packet->data;
  //*ptr = E_SERVER_TO_CLIENT_NEW_ENTITY; ptr += sizeof(uint8_t);
  //memcpy(ptr, &ent, sizeof(Entity)); ptr += sizeof(Entity);
  Bitstream bs(packet);
  bs.write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bs.write(ent);
  assert(bs.checkIsEnd());

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  //uint8_t *ptr = packet->data;
  //*ptr = E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY; ptr += sizeof(uint8_t);
  //memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  Bitstream bs(packet);
  bs.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bs.write(eid);
  assert(bs.checkIsEnd());

  enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y, size_t diameter, size_t score)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float) + 2 * sizeof(size_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  //uint8_t *ptr = packet->data;
  //*ptr = E_CLIENT_TO_SERVER_STATE; ptr += sizeof(uint8_t);
  //memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  //memcpy(ptr, &x, sizeof(float)); ptr += sizeof(float);
  //memcpy(ptr, &y, sizeof(float)); ptr += sizeof(float);
  Bitstream bs(packet);
  bs.write(E_CLIENT_TO_SERVER_STATE);
  bs.write(eid);
  bs.write(x);
  bs.write(y);
  bs.write(diameter);
  bs.write(score);
  assert(bs.checkIsEnd());

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, size_t diameter, size_t score)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float) + 2 * sizeof(size_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  //uint8_t *ptr = packet->data;
  //*ptr = E_SERVER_TO_CLIENT_SNAPSHOT; ptr += sizeof(uint8_t);
  //memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  //memcpy(ptr, &x, sizeof(float)); ptr += sizeof(float);
  //memcpy(ptr, &y, sizeof(float)); ptr += sizeof(float);
  Bitstream bs(packet);
  bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.write(eid);
  bs.write(x);
  bs.write(y);
  bs.write(diameter);
  bs.write(score);
  assert(bs.checkIsEnd());

  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  //uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  //ent = *(Entity*)(ptr); ptr += sizeof(Entity);
  assert(packet->dataLength >= sizeof(MessageType));
  Bitstream bs(packet->data + sizeof(MessageType), packet->dataLength - sizeof(MessageType));
  // the first elem is from MessageType and we should skip it
  bs.read(ent);
  assert(bs.checkIsEnd());
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  //uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  //eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  assert(packet->dataLength >= sizeof(MessageType));
  Bitstream bs(packet->data + sizeof(MessageType), packet->dataLength - sizeof(MessageType));
  // the first elem is from MessageType and we should skip it
  bs.read(eid);
  assert(bs.checkIsEnd());
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y, size_t& diameter, size_t& score)
{
  //uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  //eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  //x = *(float*)(ptr); ptr += sizeof(float);
  //y = *(float*)(ptr); ptr += sizeof(float);
  assert(packet->dataLength >= sizeof(MessageType));
  Bitstream bs(packet->data + sizeof(MessageType), packet->dataLength - sizeof(MessageType));
  // the first elem is from MessageType and we should skip it
  bs.read(eid);
  bs.read(x);
  bs.read(y);
  bs.read(diameter);
  bs.read(score);
  assert(bs.checkIsEnd());
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, size_t& diameter, size_t& score)
{
  //uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  //eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  //x = *(float*)(ptr); ptr += sizeof(float);
  //y = *(float*)(ptr); ptr += sizeof(float);
  assert(packet->dataLength >= sizeof(MessageType));
  Bitstream bs(packet->data + sizeof(MessageType), packet->dataLength - sizeof(MessageType));
  // the first elem is from MessageType and we should skip it
  bs.read(eid);
  bs.read(x);
  bs.read(y);
  bs.read(diameter);
  bs.read(score);
  assert(bs.checkIsEnd());
}

