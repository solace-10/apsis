#pragma once

#include "core/log.hpp"

//////////////////////////////////////////////////////////////////////////
// PANDORA_ASSERT / PANDORA_VERIFY
//
// Both report failures through Log::Error(), which logs to every registered
// log target before calling psnip_trap() and exiting. Trapping through the
// log rather than through <cassert> matters on the web build: the trap breaks
// on the failing line in the debugger, and the message reaches the in-game
// log instead of being swallowed by abort().
//
// Because Log::Error() returns a Log::Stream, extra context can be appended:
//     PANDORA_ASSERT(x < m_Width) << "x=" << x;
//
// PANDORA_ASSERT is compiled out in release builds and its expression is then
// never evaluated. PANDORA_VERIFY always evaluates its expression and always
// aborts on failure, so it suits cheap invariants on cold paths.
//////////////////////////////////////////////////////////////////////////

#if defined(NDEBUG)
// The expression is kept in unreachable code so that variables used only
// inside an assert don't become unused in release builds.
#define PANDORA_ASSERT(expr)  \
    if (true)                 \
    {                         \
    }                         \
    else if (expr)            \
    {                         \
    }                         \
    else                      \
        ::WingsOfSteel::Log::Error()
#else
#define PANDORA_ASSERT(expr)                                                             \
    if (expr)                                                                            \
    {                                                                                    \
    }                                                                                    \
    else                                                                                 \
        ::WingsOfSteel::Log::Error() << "Assertion failed: " #expr " @ " __FILE__ ":" << __LINE__ << " "
#endif

#define PANDORA_VERIFY(expr)                                                             \
    if (expr)                                                                            \
    {                                                                                    \
    }                                                                                    \
    else                                                                                 \
        ::WingsOfSteel::Log::Error() << "Verify failed: " #expr " @ " __FILE__ ":" << __LINE__ << " "
