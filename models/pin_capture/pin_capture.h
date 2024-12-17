#ifndef YARN_MODELS_PIN_CAPTURE_H_
#define YARN_MODELS_PIN_CAPTURE_H_

#include <systemc.h>

#include <string>

#include "base/channel.h"
#include "libs/scp/report/include/scp/report.h"
#include "models/period_generator/period_generator.h"

namespace state_bus {
struct Transaction {
  uint32_t boc_a_count;
  uint32_t in_keep_alive;
};

typedef yarn::ChannelGet<Transaction> Pipeline;
}  // namespace state_bus

namespace pin_capture {

// Abstract top transaction layer. This is where the meat of the code goes.
// Responsible for calculating transaction based on BOC cycles
class Reference : ::sc_core::sc_module {
 public:
  Reference(::sc_core::sc_module_name);
  void Start();
  std::string Name() const { return std::string(name()); }

 private:
  const std::string name_;
  SCP_LOGGER();

 protected:
  virtual const period_generator::Transaction AwaitBoc() = 0;
  virtual const state_bus::Transaction GetStateBusTransaction() = 0;
  // FIXME: Add GetMethod's for other Pin Capture interfaces
};

// ReferenceAgent sits in-between the Reference Model and the Wire layer (that defines the fiels of a transaction)
// There can be several different agents depending on where the Reference Model is used (Block, System, Stand alone,
// Google Test etc.)
class ReferenceAgent : public Reference {
 private:
  period_generator::Pipeline& per_gen_;
  state_bus::Pipeline& state_bus_pipeline_;
  state_bus::Transaction current_transaction_;

 public:
  ReferenceAgent(::sc_core::sc_module_name, period_generator::Pipeline& period_generator,
                 state_bus::Pipeline& state_bus_pipeline);

 protected:
  const period_generator::Transaction AwaitBoc() override;
  const state_bus::Transaction GetStateBusTransaction() override;
};

// FIXME: This should be replaces with google MOCK class
class GoogleTestReferenceAgent : public Reference {
 public:
  GoogleTestReferenceAgent(::sc_core::sc_module_name name) : Reference{name}, boc_count_(0) {};
  const period_generator::Transaction AwaitBoc() override;
  const state_bus::Transaction GetStateBusTransaction() override;
  uint32_t boc_count_;
};

struct Transaction {
  uint32_t data;
};

typedef yarn::ChannelGet<Transaction> Pipeline;
}  // namespace pin_capture

#endif  // YARN_MODELS_PIN_CAPTURE_H_