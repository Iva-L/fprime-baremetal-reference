// ======================================================================
// \title  Led.hpp
// \author ivanlara
// \brief  hpp file for Led component implementation class
// ======================================================================

#ifndef LedBlinker_Led_HPP
#define LedBlinker_Led_HPP

#include "FprimeBaremetalReference/Components/Led/LedComponentAc.hpp"

namespace LedBlinker {

class Led final : public LedComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct Led object
    Led(const char* const compName  //!< The component name
    );

    //! Destroy Led object
    ~Led();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command TODO
    //!
    //! TODO
    void TODO_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                         U32 cmdSeq            //!< The command sequence number
                         ) override;
};

}  // namespace LedBlinker

#endif
