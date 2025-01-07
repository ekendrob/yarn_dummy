#include <gmock/gmock.h>

#include "gtest/gtest.h"
#include "libs/scp/report/include/scp/report.h"
#include "models/pin_capture/pin_capture.h"
#include "systemc.h"

// SystemC has its own `main` and the entry point needs to be sc_main
// So we need to initialize GoogleTest here
// The `RUN_ALL_TESTS()` will call the TEST below so there should be no need for
// A more complex TB.
int sc_main(int argc, char* argv[]) {
  std::cout << "Running sc_main() from " << __FILE__ << std::endl;
  testing::InitGoogleTest(&argc, argv);
  const std::string logfile = "/tmp/scp_smoke_report_test." + std::to_string(getpid());
  scp::init_logging(scp::LogConfig()
                    .logLevel(scp::log::DEBUG) // set log level to debug
                    .msgTypeFieldWidth(20)
                    .fileInfoFrom(5)
                    .logAsync(false)
                    .printSimTime(false)
                    .logFileName(logfile));
  // make the msg type column a bit tighter
  return RUN_ALL_TESTS();
}

namespace {
using ::testing::Return;

class MockPinCapture : public pin_capture::Reference {
public:
  MockPinCapture() : Reference("MockPinCapture") {
  };
  MOCK_METHOD((period_generator::Transaction), AwaitBoc, (), (override));
  MOCK_METHOD((state_bus::Transaction), GetStateBusTransaction, (), (override));
};

TEST(pin_capture_tests, name) {
  MockPinCapture pin_capture;
  EXPECT_EQ("MockPinCapture", pin_capture.Name());
}

TEST(pin_capture_tests, minimal_pattern_with_default_period) {
  period_generator::Transaction boc_halted = {.is_halted = true};
  period_generator::Transaction boc_running = {.is_running = true};
  MockPinCapture pin_capture;
  EXPECT_CALL(pin_capture, AwaitBoc())
      .Times(3)
      .WillOnce(Return(boc_halted))
      .WillOnce(Return(boc_running))
      .WillRepeatedly(Return(boc_halted));
  pin_capture.Start();
}
} // namespace