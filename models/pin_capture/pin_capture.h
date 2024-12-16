#ifndef YARN_MODELS_PIN_CAPTURE_H_
#define YARN_MODELS_PIN_CAPTURE_H_

#include <systemc.h>

#include <string>

#include "base/channel.h"
#include "models/period_generator/period_generator.h"

namespace state_bus {
struct Transaction {
  uint32_t boc_a_count;
  uint32_t in_keep_alive;
};

typedef yarn::channel_get<Transaction> Pipeline;
}  // namespace state_bus

namespace pin_capture {

class Reference : ::sc_core::sc_module {
 public:
  Reference(::sc_core::sc_module_name, const std::string name);
  void start();

 private:
  const std::string name_;

 protected:
  virtual const period_generator::Transaction AwaitBoc() = 0;
  virtual const state_bus::Transaction GetStateBusTransaction() = 0;
  // FIXME: Add GetMethod's for other Pin Capture interfaces
};

class ReferenceAgent : Reference {
 private:
  period_generator::Pipeline& per_gen_;
  state_bus::Pipeline& state_bus_pipeline_;
  state_bus::Transaction current_transaction_;

 public:
  ReferenceAgent(::sc_core::sc_module_name, std::string name, period_generator::Pipeline& period_generator,
                 state_bus::Pipeline& state_bus_pipeline);

 protected:
  const period_generator::Transaction AwaitBoc() override;
  const state_bus::Transaction GetStateBusTransaction() override;
};

}  // namespace pin_capture

#endif  // YARN_MODELS_PIN_CAPTURE_H_