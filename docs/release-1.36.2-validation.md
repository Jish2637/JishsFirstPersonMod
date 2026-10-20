# Release 1.36.2 validation

- The supplied client ZIP is published unchanged: SHA-256 5a0f85fcf2584a3cf6de2bde73b8e6bd6caf3d42b3d914d32e5946be89ca5daa.
- The only entry changed from the tested native-head-look-test2 ZIP is the launcher manifest (public version 1.36.2). All three binary files are identical to the tested/staged binaries.
- All runtime source, vendor declarations and regression sources match the build that passed **47/47 tests**. See native-head-look-build-and-tests.txt. This publication does not claim a fresh full recompilation of the supplied binary.
- The source project, package default, public manifest and current docs use 1.36.2. Internal compiled build labels deliberately retain the tested name, preserving alignment with the user-supplied archive.
- The updater package auditor was compiled against this source and accepted the supplied ZIP: version 1.36.2, seven owned files, verified checksum/size/layout, no source ZIP or test INI in the install package.
- The official 2.1.1 native audit passed during preparation of the matching build; no native addresses or ABI layouts changed for publication.
- User confirmed native head input passthrough and requested this package be published. New source includes the marker reset changes; no separate explicit in-game acceptance of that marker fix is claimed.
- Source is updated in GitHub; only the supplied client ZIP is attached to the release. Public tag/release will be marked stable/latest for the updater.
- After publication, the existing 1.36.0 updater will be checked against the live release from an isolated fixture. That probe downloads/verifies/stages only; it does not arm an installer or touch the game. Its output is kept separately with local publication verification.
