// ======================================================================
// \title  Stm32UartDriverTestMain.cpp
// \author ivanlara
// \brief  cpp file for Stm32UartDriver component test main function
// ======================================================================

#include "Stm32UartDriverTester.hpp"

TEST(Nominal, OpenSuccess) {
    Stm32::Stm32UartDriverTester tester;
    tester.testOpenSuccess();
}

TEST(Nominal, SendFits) {
    Stm32::Stm32UartDriverTester tester;
    tester.testSendFits();
}

TEST(Nominal, SendRejectedWhenFull) {
    Stm32::Stm32UartDriverTester tester;
    tester.testSendRejectedWhenFull();
}

TEST(Nominal, PollDrainsTx) {
    Stm32::Stm32UartDriverTester tester;
    tester.testPollDrainsTx();
}

TEST(Nominal, PollDrainsRx) {
    Stm32::Stm32UartDriverTester tester;
    tester.testPollDrainsRx();
}

TEST(Nominal, PollRxNoBuffers) {
    Stm32::Stm32UartDriverTester tester;
    tester.testPollRxNoBuffers();
}

TEST(Nominal, UartErrorRecovery) {
    Stm32::Stm32UartDriverTester tester;
    tester.testUartErrorRecovery();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
