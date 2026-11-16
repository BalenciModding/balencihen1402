# BalenciHEN 1.0.2 — restore stock home-screen panels

This build retains the BalenciHEN image, label and launch action in the PS Plus slot, as requested. All other existing BalenciHEN features are retained.

Removed source behavior:
- Shortening the managed member names m_friendPanel, m_eventBasePanel, m_communityPanel, m_mailPanel and m_partyBasePanel to suppress their panels.
- Patching createOfflinePanel and LiveAreaConfig.InitProperty to return immediately.
- Forcing creation of a developer panel at startup, plus unrelated developer-panel string edits.
- Hiding the Restart ShellUI control in HEN settings.

This allows the firmware's normal panels and offline/LiveArea behavior to run. It does not fabricate new system buttons or restore online services that the firmware/service no longer provides. The PS Plus slot intentionally remains the BalenciHEN shortcut.

## Install/test
1. Ensure Write HEN plugin files to HDD is enabled (upload_prx = 1 in /data/hen/hen.ini) so the rebuilt menu PRX replaces the older installed copy.
2. Fully restart the PS4. A browser refresh or loading a second HEN in the same boot cannot undo UI code already patched by the older plugin.
3. Extract this ZIP, run START_WINDOWS.bat, then open http://YOUR-PC-LAN-IP:8088/ on the PS4 and launch.
4. Check the home-screen row: BalenciHEN should remain in its PS Plus slot, with Friends, Events, Messages and Party restored wherever present in the stock 14.02 layout. Inspect notifications and Settings too. Compare under the same account/network conditions.
5. Report any still-missing icon by name and provide a fresh photo; build checks cannot establish visual restoration on hardware.


Validation: rebuilt kernel, installer and all five PRXs; source inspection confirms the panel-suppression calls are absent and PS Plus replacement hooks remain. The existing 19 native engine/filesystem/menu tests pass. Actual 14.02 home-screen behavior has not been verified in this session.
