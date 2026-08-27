// ======================================================================
// \title  MallocWrappers.cpp
// \brief  GNU ld `--wrap` traps for the C heap allocation family
//
// The linker is configured (see CMakeLists.txt) with:
//   -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=free
//
// This redirects every call to malloc/calloc/realloc/free in the final
// image -- including calls made from newlib internals and any third-party
// code -- to the __wrap_* definitions below instead of the real libc
// implementations (still reachable as __real_*). Per JPL flight software
// rules, no dynamic C heap allocation is permitted; every call here is
// therefore a bring-up bug and must halt execution immediately via
// FW_ASSERT rather than silently allocating or returning nullptr.
// ======================================================================
#include <Fw/Types/Assert.hpp>

#include <cstddef>

extern "C" {

void* __wrap_malloc(std::size_t size) {
    FW_ASSERT(0, static_cast<FwAssertArgType>(size));
    return nullptr;
}

void* __wrap_calloc(std::size_t num, std::size_t size) {
    FW_ASSERT(0, static_cast<FwAssertArgType>(num), static_cast<FwAssertArgType>(size));
    return nullptr;
}

void* __wrap_realloc(void* ptr, std::size_t new_size) {
    FW_ASSERT(0, static_cast<FwAssertArgType>(reinterpret_cast<PlatformPointerCastType>(ptr)),
              static_cast<FwAssertArgType>(new_size));
    return nullptr;
}

void __wrap_free(void* ptr) {
    FW_ASSERT(0, static_cast<FwAssertArgType>(reinterpret_cast<PlatformPointerCastType>(ptr)));
}

}  // extern "C"
