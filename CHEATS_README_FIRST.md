# BalenciHEN 14.02: initial JSON cheat-menu test build

Compiled from source; NOT yet tested on PS4. This is a separate console test package so the prior ordinary build remains available.

## Use
1. Fully restart the PS4. Extract this ZIP and run START_WINDOWS.bat. Open http://YOUR-PC-LAN-IP:8087/ on the PS4 and launch BalenciHEN. Do not stack this on another HEN in the same boot: this build adds a process-information command required by the menu.
2. After the menu and FTP initialize, run `py -3 upload_cheats.py PS4-LAN-IP` on the PC. This uploads the bundled database to /data/BalenciHEN/cheats/json using FTP port 2121. Alternatively upload the cheats/json directory there manually. Uploading the database does not activate cheats.
3. Launch your game and pause gameplay. Return to system Settings > BalenciHEN > BalenciHEN Cheats. This is a Settings page, not an in-game Share-button overlay.
4. The page identifies one live eboot.bin game, checks its title ID against kernel process metadata, and reads the exact game version from the existing ShellCore launch metadata. The filename and JSON id/version/process must agree. It never falls back to a different game version.
5. Choose an [OFF] cheat to enable it or an [ON] cheat to disable it. A notification reports success or the reason for refusing it. Leave and reopen the page to refresh displayed state; the original button title does not automatically refresh in place.
6. Return to the game and verify the intended effect. First test one reversible cheat for your exact game version. Disable it through the page and verify normal behavior returns.

## Implemented
- 1,883 JSON files from the official cheat repository, pinned at acba322df33996d779599306d586c420144e55ad. Files, author credits and license preserved.
- JSON checkbox patches using on/off byte strings and module-relative hex offsets.
- Named executable mapping resolution from the running game, rather than a hardcoded image base.
- Expected-byte checks before any writes, mapped-range checks, verified writes, overlap rejection, saved-byte restoration, and rollback on a failed multi-patch operation. A failed rollback locks the engine until the game/session is restarted.
- Uses HEN syscalls 107/108/109, exposing the existing process-info handler through command 8. The VM-map handler now skips its list sentinel, respects capacity and returns actual entry count.
- BalenciHEN branding and logo, FTP 2121, raw payloader 9090, existing plugin manager and optional UI metadata probe retained.
- User plugin paths remain /data/BalenciHEN/plugins and /data/BalenciHEN/plugins.ini.

## Limits
- NOT GoldHEN feature parity. No in-game Share/PS shortcut or overlay yet.
- SHN, MC4, button/one-shot entries, section-indexed memory, pointer chains, freeze loops, non-eboot processes and code-cave allocation are not implemented.
- Patches outside identified existing executable mappings are rejected. Some repository files need those unsupported capabilities even if their entries are JSON checkboxes. A matching file does not guarantee support or correctness.
- Only the exact CUSAxxxxx_XX.XX.json filename is selected. Alternative suffix variants are not automatically chosen. No cheats are automatically enabled, and the database is not automatically downloaded at console startup.
- Enabled state and original-byte snapshots live in the ShellUI plugin's memory. Restart the GAME after a ShellUI restart or HEN reload; do not assume active patches were reverted. There is no persistent enable list or automatic restore during a process crash.
- Process mapping, live identity, integration hooks, pause behavior and all gameplay effects need real 14.02 testing. A successful PC test or build does not establish console compatibility.
- The UI-color probe remains optional and off without its marker; this build does not recolor system text.

## Checks completed
Ten native PC tests cover enable/disable, exact-version rejection, expected-byte mismatch, drift on disable, rollback, overlap rejection, invalid/unsupported formats, unmapped access, and a full mocked process -> metadata -> menu -> toggle/restore path with XML escaping and stale-session rejection. These mocks do not exercise the PS4 kernel or ShellUI.
Kernel, installer and all five PRX modules are rebuilt from source. Host syntax/cache/local HTTP and output checksums are checked separately.

Run `python3 -m unittest discover -v -s tests` for native engine/menu tests. Source/rebuild instructions remain in README_FIRST.md. Third-party cheat data is GPL-3.0; see cheats/REPOSITORY_LICENSE and its README. Parser is cJSON v1.7.19, with its MIT license beside the source.
