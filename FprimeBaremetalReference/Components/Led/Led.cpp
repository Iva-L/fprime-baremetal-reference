// ======================================================================
// \title  Led.cpp
// \author ivanlara
// \brief  cpp file for Led component implementation class
// ======================================================================

#include "FprimeBaremetalReference/Components/Led/Led.hpp"

namespace LedBlinker {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

Led ::Led(const char* const compName) : LedComponentBase(compName) {}

Led ::~Led() {}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void Led ::BLINKING_ON_OFF_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::On& onOff) {
    this->m_ticksSinceToggle = 0;   // Reset count on any successful command
    this->m_blinkingState = onOff;  // Update blinking state

    this->log_ACTIVITY_HI_SetBlinkingState(onOff);

    // TODO: Report the blinking state (onOff) on channel BlinkingState.
    // NOTE: This telemetry channel will be added during the "Telemetry" exercise.

    // Provide command response
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace LedBlinker
