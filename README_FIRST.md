# BalenciHEN 1.0.2 — updated build

The cheat menu, engine, bundled database and uploader have been removed at the user's request. No cheat code is compiled or bundled.

Retained: BalenciHEN logo and PS Plus-slot shortcut, restored stock home-screen panels, plugin manager, FTP 2121, raw payload receiver 9090, Fortnite local-file reset and optional UI metadata probe. User plugins remain under /data/BalenciHEN/plugins; their configuration is /data/BalenciHEN/plugins.ini.

Fully restart PS4, extract this ZIP and run START_WINDOWS.bat. Open http://YOUR-PC-LAN-IP:8088/ on PS4 and launch. Keep Write HEN plugin files to HDD enabled (upload_prx = 1 in /data/hen/hen.ini), so the old installed menu module is replaced.

Previously uploaded JSON files on the console are not deleted automatically. They are no longer used by this build. If unwanted, remove /data/BalenciHEN/cheats through FTP after checking its contents. Do not delete /data/BalenciHEN/plugins or plugins.ini.

Version remains 1.0.2. Compiled/build checks are not evidence of PS4 compatibility. This rebuild needs console verification. See HOME_RESTORE_README_FIRST.md, FORTNITE_README_FIRST.md and UI_COLOR_README.md for retained feature behavior and limitations.

Native verification: nine Fortnite filesystem tests. Rebuild source/rebuild.sh using the dependencies documented in PROVENANCE.md: PS4SDK points to the payload SDK, OO_PS4_TOOLCHAIN to OpenOrbis; GCC/binutils, Clang/LLD, Python 3 and xxd are required. No full SDK/toolchain is bundled.
