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

void Led ::TODO_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace LedBlinker
