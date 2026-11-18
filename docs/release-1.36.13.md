# Input blocking on ReSkate Build 20261116

ReSkate can forward game controller polls through its own input wrappers. The module-wide bypass in 1.36.12 could treat those as private controller reads and let game input through. Version 1.36.13 filters each outer XInput poll once and scopes the real controller readers so sampling, recording and navigation receive physical input.

Raw Input and HID setup also proceeds when XInput readers are unavailable. Regression coverage exercises forwarded polls, private physical reads, stick capture and menu capture. All 51 automated tests passed. Live gameplay testing of this patch is pending.
