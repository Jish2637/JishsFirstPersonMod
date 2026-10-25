# 1.36.4 — independent core startup

The reported 2.1.2 build identities and address signatures matched. The latest
logs stopped just after `Jump episode contracts verified`, before slow DirectInput
device discovery could return. The original call graph was:

`bootstrap -> install_hooks -> install_stick_runtime -> attach_stick_runtime -> install_stick_input_filter -> install_stock_camera_feed -> start_menu`.

The corrected startup order is:

`bootstrap -> install_hooks (camera / motion / source feed) -> start_menu -> compatibility::enable + initialization status=2 -> experimental emote aim, MSC presence, updater -> optional input filters`.

DirectInput enumerates real attached drivers. It cannot be safely cancelled while
inside a device driver's API call; instead the camera/settings UI become functional
before that call runs. Additional log entries bracket per-interface enumeration,
CreateDevice calls and transaction timing. The last logged step can now identify
a slow driver. If optional filtering fails, some modifier/gesture filtering may be
unavailable, but core camera and Insert menu remain functional.

**Not tested in game here.** A Windows MSVC build and in-game checks remain necessary.
Do not treat compilation/version strings alone as runtime validation.
