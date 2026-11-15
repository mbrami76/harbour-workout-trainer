# Security policy

## Supported versions

Only the latest release receives fixes.

## Reporting a vulnerability

Please do not open a public issue for a security problem. Use GitHub's private reporting instead: open the **Security** tab of the repository and choose **Report a vulnerability**. Include the app version, the Sailfish OS version and the steps to reproduce.

This is a volunteer project: reports are handled on a best-effort basis, and a fix is published as a new release. Credit is given in the release notes if you want it.

## What the application can see and do

Knowing this helps to judge what counts as a vulnerability:

- The application runs in the Sailfish OS sandbox with two permissions: `Audio` (sound cues) and `Documents` (export, import and the file pickers).
- It makes **no network connections** and has no background service and no D-Bus interface of its own.
- Workouts, exercises, the session log and settings are stored in the application's own data directory.
- Export files are written to `~/Documents/WorkoutTrainer` as plain JSON. They contain the workouts and the exercise texts, nothing else.
- Import reads only the file the user picks.

## In scope

- Data leaving the device, or being written outside the locations above.
- A crafted import file that crashes the application, corrupts the stored data or overwrites existing data without confirmation.
- Memory-safety or parsing errors in the import code.
- Problems in the packaging (file permissions, scriptlets, requested permissions).

## Out of scope

- Anything that needs an attacker who already controls the user's session or has root access to the device.
- Weaknesses of Sailfish OS or of Qt themselves.

## Verifying releases

Releases are not signed. Each release page lists the SHA-256 checksums of the packages; compare them with `sha256sum` before installing.
