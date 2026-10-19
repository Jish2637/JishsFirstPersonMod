# Release 1.36.0 validation

- Release build: PASS; 43/43 tests passed.
- Native 2.1.1 binary contract audit: PASS (release-1.36.0-native-audit.json).
- All src files match the user-confirmed updater-test2 build; only the updater HTTP user-agent version changed to 1.36.0.
- User confirmed updater-test2 in-game behavior, green notification and revised wording worked.
- First build check exposed two fixture paths mistakenly advanced to the unpublished 1.36.0 ZIP. Restored their pinned 1.35.0 fixture paths; full rebuild and all checks passed. No runtime code changes resulted.
- Game installation untouched during release preparation.
- Only the client ZIP will be uploaded as a release asset; matching source is committed to GitHub.
