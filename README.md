# Jish's First Person Mod 1.34.0 — ReSkate 2.1.0

Dedicated ReSkate 2.1.0 build based on released 1.34.0. The user confirmed the FP-removed compat-test2 package works in-game. This tagged source preserves that tested binary’s build inputs and internal compatibility label.

The camera algorithms, configured offsets and confirmed head-position fix are retained. The migration updates private overlay, model, controller and spectator contracts against pinned official source and the exact official DLL. Native entry signatures and release layouts are checked separately from behavioral tests.

Insert opens the stock menu and companion. F9 hides/shows the companion inside the stock menu. Standalone F9 opening remains disabled. Use mouse and keyboard in the companion; stock ReSkate owns controller navigation. Explicit gameplay-binding recording temporarily captures the chord and then restores stock navigation. Controller panel-opening shortcuts remain disabled.

The MSC [FP] nametag implementation has been removed. This build performs no MSC capability discovery, installs no MSC command hook and sends no first-person presence commands or heartbeats.

Settings, presets and logs remain under scripts/Custom/FirstPerson. Existing preservation/migration behavior remains; no settings reset is required.

See [installation](LAUNCHER_INSTALL.md), [menu controls](CONTROLLER_MENU.md), [build and provenance](docs/MIGRATION_210.md), and [automated results and live checklist](docs/COMPATIBILITY_210_TEST_REPORT.md). Older feature documents describe retained features; historical test/release claims in them are not validation of this build.

Build with ./build.ps1 using Visual Studio 2026 C++ tools and Windows SDK. Package with ./package.ps1. Both operate inside this source directory. The layout probes additionally require pinned upstream source, and the 2.1.0 binary contract test requires ../upstream/ReSkate-2.1.0.dll.

After installing this update, reapply your preferred preset. Existing preset files are preserved.
