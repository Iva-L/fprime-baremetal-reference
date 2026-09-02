// ======================================================================
// \title  CacheMaintenance.hpp
// \brief  D-cache clean/invalidate wrappers for Cortex-M7 DMA buffers
// ======================================================================
#ifndef FPRIME_STM32_CACHE_MAINTENANCE_HPP
#define FPRIME_STM32_CACHE_MAINTENANCE_HPP

#include <cstddef>
#include <cstdint>

#include "stm32h7xx_hal.h"

namespace Stm32 {

//! Clean D-cache over [addr, addr + size) before starting a memory-to-peripheral
//! DMA transfer, so the DMA controller reads data the CPU has actually written.
//! addr/size must be 32-byte aligned/sized to avoid touching unrelated data,
//! since the cache line granularity is fixed at 32 bytes on the Cortex-M7.
inline void CleanDCacheForDma(const void* addr, std::size_t size) {
    SCB_CleanDCache_by_Addr(const_cast<uint32_t*>(reinterpret_cast<const uint32_t*>(addr)),
                            static_cast<int32_t>(size));
}

//! Invalidate D-cache over [addr, addr + size) after a peripheral-to-memory
//! DMA transfer completes, so the CPU reads data the DMA controller actually
//! wrote instead of a stale cache line. Same alignment requirement as above.
inline void InvalidateDCacheForDma(void* addr, std::size_t size) {
    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(addr), static_cast<int32_t>(size));
}

}  // namespace Stm32

#endif
