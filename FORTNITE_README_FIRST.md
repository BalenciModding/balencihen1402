# Fortnite Unbanner — local file reset

New menu button: Settings > BalenciHEN > Fortnite Unbanner.

This implements the two local file removals described in the supplied screenshot by moving the original files into a recovery folder. It DOES NOT verify or report removal of an Epic/Fortnite ban, change console hardware identifiers, modify accounts, or contact Epic. The button name was requested by the user; its description and completion notice identify the actual operation.

## Test
1. Fully restart the PS4. Extract this ZIP, run START_WINDOWS.bat and open http://YOUR-PC-LAN-IP:8088/ on PS4 to launch this build.
2. Close Fortnite completely and pause any Fortnite installation/download/update tasks.
3. Open Settings > BalenciHEN > Fortnite Unbanner. Read the confirmation text, then run it if you intend to reset these local files.
4. The notification reports what happened and the recovery folder. Verify the file locations through FTP. Only a separate console/game test can establish whether this changes your specific Fortnite issue; no successful unban is claimed.

## Exact behavior
The code finds exactly one installed title whose /user/appmeta/CUSAxxxxx/param.sfo declares TITLE=Fortnite (optional trademark suffix) and a matching TITLE_ID. No guessed CUSA ID is embedded. It checks the running process list and refuses to proceed if that Fortnite title is still running or process state cannot be verified.

It requires both regular, non-symlink files:
/user/download/CUSAxxxxx/download0.dat
/user/download/CUSAxxxxx/download0_information.dat

Both are moved to:
/user/download/CUSAxxxxx/.BalenciHEN-backup-TIMESTAMP-PID-N/

No other files are moved. The directory is hidden by its leading dot; enable hidden-file display in your FTP client. If the second move fails, the code attempts to restore the first file without overwriting a newly recreated file. If restoration fails, the error names the recovery folder for manual recovery. Backups are never automatically deleted.

To recover, close Fortnite, pause downloads, inspect the backup directory, and copy its two files back to their original locations only after checking whether replacement files have since been created. Do not overwrite newer download data blindly.

Missing files, ambiguous installations, mismatched/malformed metadata, symlinked paths and unknown process state produce a refusal message. If metadata uses a different title string/layout, the code may not identify the installation; it does not fall back to resetting unrelated titles.

## Validation
All 19 native tests passed, including nine filesystem cases for exact-pair handling, unaffected unrelated data, rollback, missing files, symlinks, wrong/malformed title metadata, ambiguous installations and a running game. PS4 installer and PRXs compile and link. Actual PS4 UI, filesystem, process and Fortnite behavior remain unverified.

