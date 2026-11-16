# BalenciHEN red system text: optional discovery build

This build DOES NOT recolor text. It adds a one-shot, read-only metadata probe to the menu plugin so we can identify the actual loaded 14.02 theme/font methods instead of guessing function names or addresses. PS4 runtime behavior is unverified. Keep the ordinary BalenciHEN build as your baseline.

Use this only after the ordinary BalenciHEN build's menu and FTP are confirmed working:
1. Through FTP, place the included empty enable_ui_probe file at /data/BalenciHEN/enable_ui_probe.
2. Fully restart the console. Start this package's local host (START_WINDOWS.bat; port 8088) and launch this build once.
3. After its initialization, retrieve /data/BalenciHEN/ui_color_probe.txt through FTP and provide it for analysis.

The marker is consumed before inspection, so the probe does not automatically run again on a subsequent boot. Without the marker, this build skips metadata inspection. It does not modify the active theme or system UI colors. If initialization fails, return to the baseline after a full reboot and report the final stage.

Expected evidence: a file with ASSEMBLY/CLASS entries, theme/font/color method and property names, and an END line. That identifies candidates only; it does not establish method signatures, a working color override or coverage of every system screen. If there is no file, the hook may not have run or file access may have failed; this is not evidence that color APIs are absent.

The probe enumerates loaded app/Sce.* assembly metadata, capped at 6000 lines and 256 assemblies, with no UI setters or managed method execution. No application titles, user names, text contents or account data are intentionally collected; inspect the file before sharing.

Theme research already confirms the ARGB theme input <fontcolor>#FFFF0000</fontcolor>. Applying that through a custom theme is distinct from automatically changing the active theme via BalenciHEN. The proposed integration is an optional red/default setting in the existing menu plugin after a real method/property and its signature have been verified. Any screens that bypass the theme need separate coverage testing.

Sources: https://github.com/speedrod55/PS4DynamicThemeSample/blob/main/theme.xml ; https://github.com/OSM-Made/PS4-Mono ; local source/plugins/plugin_mono.
