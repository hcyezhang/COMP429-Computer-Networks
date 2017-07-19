#ifndef PORT_H
#define PORT_H

typedef struct Port {
  uint16_t number;
  uint16_t to;
  uint32_t cost;
  uint32_t updated;
  bool connected;
} Port;

typedef struct DVCell {
  uint16_t next_hop;
  uint32_t cost;
  uint32_t updated;
} DVCell;

typedef struct NNCell {
  uint32_t cost;
  uint16_t port;
} NNCell;

#endif
