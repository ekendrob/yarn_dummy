#ifndef YARN_MODELS_PERIOD_GENERATOR_H_
#define YARN_MODELS_PERIOD_GENERATOR_H_

#include <cstdint>
#include "base/channel.h"

namespace period_generator {

struct Transaction {
  uint32_t boc_count;
  enum { BOC_A, BOC_B } type;
  bool is_halted;
  bool in_KA;
  bool is_running;
  uint32_t residue;
};

typedef yarn::ChannelGet<Transaction> Pipeline;

// So BOC cycles are transactions with a channel (Reference) that models can wait upon
// Each Reference Model will have it's own channel it can wait on.
// There should be a single central source for Period Generation and from that model there will be pipeline fanout
// to all other models

// This is a simple agent that monitors the corresponding RTL signals and drives the event that way
// class ReferenceRTLAgent : Reference {};

// Behavioral agent that models the behavior of the Period Generator
// class ReferenceSystemCAgent : Reference {};

// An agent used with GoogleTest to create the C++ test suite
// class ReferenceTestAgent : Reference {};

}  // namespace period_generator
#endif  // YARN_MODELS_PERIOD_GENERATOR_H_