// ======================================================================
// \title  Stm32UartDriverTestMain.cpp
// \author ivanlara
// \brief  cpp file for Stm32UartDriver component test main function
// ======================================================================

#include "Stm32UartDriverTester.hpp"

TEST(Nominal, toDo) {
    Stm32::Stm32UartDriverTester tester;
    tester.toDo();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
