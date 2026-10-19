# 1.34.0-compat-test2 — ReSkate 2.1.0

The MSC [FP] implementation was removed after the user reported chat spam in compat-test1. Its bootstrap, worker, Steam capability discovery, command hook and presence/heartbeat sender are removed from the source and DLL build. No server, working game installation or GitHub release was changed.

Camera algorithms, offsets, head attachment, smoothing, menu rendering and controller ownership are unchanged from compat-test1. Settings and presets retain their existing layout.

Automated validation: see CTEST_210.txt for the current 38-test run (the MSC test was removed with the feature), and NATIVE_AUDIT_210.json for the remaining native contracts. Live validation of this package is pending. Check that chat remains quiet, then verify camera-only and camera-and-character modes, bails/get-up, mounting, return-to-marker, Insert/F9, binding recording and stock controller navigation.

This client no longer implements first-person nametag reporting. Existing server-side indicators may take their normal expiry interval to clear. Server-side code is unchanged.
