#include <iostream> // to implement uint8_t
#include <assert.h>

class Bitstream
{
private:
  uint8_t *buffer;
  size_t leftSize;
public:
  Bitstream(void* dataBuffer, size_t sizeOfBuffer) : buffer((uint8_t*) dataBuffer), leftSize(sizeOfBuffer)
  {}

  Bitstream(ENetPacket* packet) : buffer(packet->data), leftSize(packet->dataLength)
  {}

  template <typename T>
  void write(const T& val)
  {
    if (leftSize < sizeof(val))
    {
        printf("Error! You don't have enough memory to write this value in Bitstream buffer.\n");
        assert(leftSize >= sizeof(val));
    }

    memcpy(buffer, &val, sizeof(val));
    buffer += sizeof(val);
    leftSize -= sizeof(val);
  }

  template <typename T>
  void read(T& val)
  {
    if (leftSize < sizeof(val))
    {
        printf("Error! You don't have enough memory to read this value in Bitstream buffer.\n");
        assert(leftSize >= sizeof(val));
    }

    memcpy(&val, buffer, sizeof(val));
    buffer += sizeof(val);
    leftSize -= sizeof(val);
  }

  bool checkIsEnd()
  {
    return leftSize == 0;
  }
};