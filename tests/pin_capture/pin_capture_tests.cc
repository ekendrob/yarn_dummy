#include "gtest/gtest.h"
#include "libs/scp/report/include/scp/report.h"
#include "models/pin_capture/pin_capture.h"
#include "systemc.h"

// SystemC has it's own `main` and the entry point needs to be sc_main
// So we need to initialize GoogleTest here
// The `RUN_ALL_TESTS()` will call the TEST below so there should be no need for
// A more complex TB.
int sc_main(int argc, char *argv[]) {
  std::cout << "Running sc_main() from " << __FILE__ << std::endl;
  testing::InitGoogleTest(&argc, argv);
  std::string logfile = "/tmp/scp_smoke_report_test." + std::to_string(getpid());
  scp::init_logging(scp::LogConfig()
                        .logLevel(scp::log::DEBUG)  // set log level to debug
                        .msgTypeFieldWidth(20)
                        .fileInfoFrom(5)
                        .logAsync(false)
                        .printSimTime(false)
                        .logFileName(logfile));  // make the msg type column a bit tighter
  return RUN_ALL_TESTS();
}

namespace {
TEST(pin_capture_tests, dummy) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

TEST(pin_capture_tests, get_name) {
  sc_clock clock("clock", 1, SC_NS);  // Create a clock signal with 1 ns period
  yarn::GTestChannel<period_generator::Transaction>  period_generator("GTest_period_generator");
  yarn::GTestChannel<state_bus::Transaction>  state_bus_pipeline("GTest_state_bus");
  pin_capture::ReferenceAgent pin_capture("pin_capture", "pin_capture_1", period_generator, state_bus_pipeline);
  EXPECT_EQ("pin_capture_1", pin_capture.Name());
}

}  // namespace
