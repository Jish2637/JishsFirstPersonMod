# Controller binding capture buffer fix

The ReSkate 2.1.2 gamepad output is 20 bytes, not 32. StockGamepad now uses fixed-width native fields and asserts the size and field offsets. The capture path clears only this structure and detects held input from 16-bit buttons/sticks and 8-bit triggers rather than invalid float axes.

Release build and all 52 automated tests passed. The new test exercises the production capture helper with a 12-byte trailing canary and an exactly sized buffer immediately before a protected page. It also checks physical buttons, triggers and stick extremes, raw binding reads, release suppression, metadata and normal input after release.

Local recording-fix test build based on 1.36.5; not published to GitHub. Close Skate before merging the client ZIP into the game folder. This client ZIP excludes dinput8.dll and preserves the existing loader. Confirm recording a controller binding no longer crashes in game.

DLL SHA256: 7fe1d9086a38e259289f0c3c510c261f3f42a12ab26d1ec70e78996f0ed14a71.
