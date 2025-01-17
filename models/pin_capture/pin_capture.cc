#include "pin_capture.h"

#include "libs/scp/report/include/scp/report.h"

pin_capture::Reference::Reference(const ::sc_core::sc_module_name&) {
  SCP_INFO() << "Constructor [" << sc_time_stamp() << "]";
  SC_METHOD(Start);
}

void pin_capture::Reference::Start() {
  // Get the next BOC
  auto boc = AwaitBoc();

  // Wait for 1st running transaction - as we will see default periods in the beginning
  while (boc.is_halted) boc = AwaitBoc();

  // Generate transactions while pattern is runnign
  while (boc.is_running) {
    // Get data from all interfaces - These methods
    state_bus::Transaction state_bus = GetStateBusTransaction();

    // FIXME: Create outgoing transaction

    // wait for next BOC Cycle
    boc = AwaitBoc();
  }
}

pin_capture::ReferenceAgent::ReferenceAgent(const ::sc_core::sc_module_name& sc_name,
                                            period_generator::Pipeline& period_generator,
                                            state_bus::Pipeline& state_bus_pipeline) :
  Reference(sc_name), per_gen_(period_generator), state_bus_pipeline_(state_bus_pipeline), current_transaction_() {
}

period_generator::Transaction pin_capture::ReferenceAgent::AwaitBoc() { return per_gen_.Get(); }

state_bus::Transaction pin_capture::ReferenceAgent::GetStateBusTransaction() { return state_bus_pipeline_.Get(); }