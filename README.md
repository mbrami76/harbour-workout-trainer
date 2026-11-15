# Workout Trainer

A workout timer for Sailfish OS for repetition-based exercises, interval (HIIT) training and circuits. It has a large countdown, sound cues and your own exercise library. It works fully offline and collects nothing.

Status: in development, not released yet.

## Important notice

The exercises, workouts and loads (durations, repetitions, rests) that come with the app, and any you create, are provided as they are, without any warranty, express or implied. They have not been reviewed by qualified professionals and, depending on the condition of each user and on how the exercise is understood and performed, they may be harmful. If in doubt, ask a physician or a qualified trainer first.

## Features (version 1)

- Workouts built from exercises: timed or repetition steps, rest, warm-up and cooldown, grouped in blocks with rounds.
- A quick builder for interval workouts, favorites and templates.
- Written instructions and form tips for every exercise, with a muscle-group picture. A starter library is included and can be edited.
- Distinct sounds for work, rest, round change and the end of the workout. Optional vibration where the device supports it.
- The screen stays on during a workout. The workout pauses when the app loses the foreground (call, lock screen, another app) and waits until you resume it.
- Export and import of workouts as JSON.
- A silent log of sessions, kept for the history and statistics screens planned for version 2.

Planned for version 2: history and statistics (calendar heatmap, most repeated exercises, muscle-group usage) and spoken cues from your own text with offline speech synthesis.

## Requirements

Sailfish OS 5.1 or later on aarch64 (phones) or i486 (emulator). The app is written to pass the Harbour validation and uses only the `Audio` and `Documents` permissions.

## Building

With the Sailfish SDK:

```
sfdk -c target=SailfishOS-5.1.0.11-aarch64 build
sfdk -c target=SailfishOS-5.1.0.11-aarch64 check RPMS/harbour-workout-trainer-*.rpm
```

Host tests (they need no device) run on the i486 target:

```
sfdk -c target=SailfishOS-5.1.0.11-i486 build-shell qmake harbour-workout-trainer.pro CONFIG+=hostbuild
sfdk -c target=SailfishOS-5.1.0.11-i486 build-shell make -j4
sfdk -c target=SailfishOS-5.1.0.11-i486 build-shell make check
```

### Speech announcements

The spoken announcements need a speech engine (sherpa-onnx and ONNX Runtime libraries, eSpeak NG data and two voices, about 50 MB installed). They are not part of this repository. They are packed into a speech bundle, one archive with a checksum and a manifest, that is attached to a release of this repository. Release builds for the phone (aarch64) use it.

Fetch and unpack it with:

```
tools/fetch-speech-bundle.sh [directory]
```

The script downloads the file named in `tools/speech-bundle.conf`, checks its SHA-256 and unpacks it into the directory (default `../build/speech-libs`, next to the source folder). It does nothing if that directory already holds the bundle. The unpacked directory is the speech root: `RT_SPEECH_ROOT` names it (an absolute path); if the variable is not set, the project file looks in `../../build/speech-libs` relative to the `app` folder. The qmake output states whether the engine is on and which root was used.

A build without the bundle, and every build for i486, has no speech engine: it builds and runs normally, and the spoken announcements are hidden. The release notes of the release that carries the bundle list its contents, versions, licences, size and checksum.

## Documentation

- [Requirements](docs/requirements.md)
- [Architecture](docs/architecture.md)

## Privacy

The app makes no network connections. Workouts, exercises and the session log are stored in the app's own data directory. Exported files go to `~/Documents/WorkoutTrainer`.

## Development

The development of this project included generative AI tools.

## Licence

GPL-3.0-or-later. The muscle-group pictures are released under CC0 1.0. The digit font DSEG7 Classic by keshikan is licensed under the SIL Open Font License 1.1; its licence text is in `app/fonts/DSEG-LICENSE.txt` and ships with the app.
