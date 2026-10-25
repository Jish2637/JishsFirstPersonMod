// Offline checks for the universal loader's dinput8_mods.txt sync (mod_list.h).
#include "loader/mod_list.h"
#include <cstdio>
#include <cstdlib>

using namespace universal_loader;
using V = std::vector<std::wstring>;
static int failures = 0;
static void check(bool ok, const char* what) { if (!ok) { std::printf("FAIL: %s\n", what); ++failures; } }

int main() {
    // First launch: header plus every DLL found, sorted; the loader itself is never listed.
    auto r = sync({}, false, {L"b.dll", L"A.dll", L"dinput8.dll", L"notes.txt"});
    check(r.lines.size() == header_lines().size() + 2, "created list has header and two mods");
    check(r.load == V{L"A.dll", L"b.dll"}, "first launch loads all found mods sorted");
    check(r.added == V{L"A.dll", L"b.dll"}, "first launch reports additions");

    // Existing list: order, comments and switched-off lines kept; new DLL appended; deleted DLLs removed.
    const V existing{L"# my comment", L"Second.dll", L"# Off.dll", L"Gone.dll", L"First.dll", L"", L"#   GoneOff.dll"};
    r = sync(existing, true, {L"First.dll", L"Second.dll", L"Off.dll", L"New.dll"});
    check(r.lines == V{L"# my comment", L"Second.dll", L"# Off.dll", L"First.dll", L"", L"New.dll"}, "sync keeps order, appends and removes");
    check(r.load == V{L"Second.dll", L"First.dll", L"New.dll"}, "switched-off mod not loaded");
    check(r.added == V{L"New.dll"}, "one addition");
    check(r.removed == V{L"Gone.dll", L"GoneOff.dll"}, "two removals including a switched-off one");

    // Case-insensitive matching, duplicates dropped, nothing to change -> unchanged.
    r = sync({L"mod.DLL", L"MOD.dll"}, true, {L"Mod.dll"});
    check(r.lines == V{L"mod.DLL"} && r.removed == V{L"MOD.dll"}, "duplicate removed case-insensitively");
    r = sync({L"Mod.dll"}, true, {L"Mod.dll"});
    check(!r.changed(), "no change when list matches folder");

    // Entries with folders are comments as far as sync is concerned (left alone).
    r = sync({L"sub\\x.dll"}, true, {});
    check(r.lines == V{L"sub\\x.dll"} && r.load.empty(), "path entries ignored, not loaded");

    std::printf(failures ? "%d failure(s)\n" : "all mod list checks passed\n", failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
