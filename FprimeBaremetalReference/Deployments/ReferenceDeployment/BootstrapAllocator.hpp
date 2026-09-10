// ======================================================================
// \title  BootstrapAllocator.hpp
// \brief  Fixed-pool allocator used during deployment initialization
// ======================================================================
#ifndef REFERENCEDEPLOYMENT_BOOTSTRAPALLOCATOR_HPP
#define REFERENCEDEPLOYMENT_BOOTSTRAPALLOCATOR_HPP

#include <Fw/Types/MemAllocator.hpp>

namespace ReferenceDeployment {

Fw::MemAllocator& getBootstrapAllocator();
void lockBootstrapAllocator();

}  // namespace ReferenceDeployment

extern "C" void ReferenceDeployment_registerBootstrapAllocator();

#endif  // REFERENCEDEPLOYMENT_BOOTSTRAPALLOCATOR_HPP
