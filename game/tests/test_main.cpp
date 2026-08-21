#include <catch2/catch_session.hpp>

#include <core/log.hpp>

// Catch2's own main is not used, so that pandora's logging can be brought up before any test runs.
//
// This matters because of how PANDORA_ASSERT reports: it writes through Log::Error(), which logs to
// every registered target and then calls psnip_trap() and exit(1). An assertion tripped inside a
// test therefore kills the process outright - Catch2 never gets the chance to report it, and cannot
// catch it either. Registering a StdOutLogger is what turns that from a silent non-zero exit into a
// message naming the failed expression and its file and line.
//
// CTest is what makes such a death survivable: game/tests/CMakeLists.txt registers each test case
// as its own CTest entry, so a trap takes down one case rather than the whole run.
//
// Note also that PANDORA_ASSERT compiles out under NDEBUG, so the suite is only meaningful in a
// debug build. ./scripts/test.sh always configures Debug for that reason.
int main(int argc, char* argv[])
{
    using namespace WingsOfSteel;

    Log::AddLogTarget(std::make_shared<StdOutLogger>());

    return Catch::Session().run(argc, argv);
}
