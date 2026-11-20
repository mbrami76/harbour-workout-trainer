# Workout Trainer

A workout timer for Sailfish OS for repetition-based exercises, interval (HIIT) training and circuits. It has a large countdown, sound cues and your own exercise library. It works fully offline and collects nothing.

Status: released.

## Important notice

The exercises, workouts and loads (durations, repetitions, rests) that come with the app, and any you create, are provided as they are, without any warranty, express or implied. They have not been reviewed by qualified professionals and, depending on the condition of each user and on how the exercise is understood and performed, they may be harmful. If in doubt, ask a physician or a qualified trainer first.

## Screenshots

|01|02|03|04|
|---|---|---|---|
| ![][01] | ![][02] | ![][03] |![][04]|

|01|02|03|04|
|---|---|---|---|
| ![][05] | ![][06] | ![][07] |![][08]|


## Features (version 1)

- Workouts built from exercises: timed or repetition steps, rest, warm-up and cooldown, grouped in blocks with rounds.
- A quick builder for interval workouts, favorites and templates.
- Written instructions and form tips for every exercise, with a muscle-group picture. A starter library is included and can be edited.
- Distinct sounds for work, rest, round change and the end of the workout. Optional vibration where the device supports it.
- The screen stays on during a workout. The workout pauses when the app loses the foreground (call, lock screen, another app) and waits until you resume it.
- Export and import of workouts as JSON.
- A silent log of sessions, kept for the history and statistics screens planned for version 2.
- Staistics
- Spoekn exercise cues with custom text, using internal offline TTS engine.

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

The spoken announcements need a speech engine (sherpa-onnx and ONNX Runtime libraries, eSpeak NG data and two voices, about 50 MB unpacked). They are not part of this repository. They are packed into a speech bundle, one archive with a checksum and a manifest, that is attached to a release of this repository (the release with the tag `speech-bundle-1`). Builds for the phone (aarch64) use it; its release notes list the contents, versions, licences, size and checksum.

To use it, download the archive, check its SHA-256 against the one in the release notes and unpack it into any folder (steps are in the release notes). That folder is the speech root: set the environment variable `RT_SPEECH_ROOT` to its absolute path when you build for aarch64. If the variable is not set, the project file looks for the folder `build/speech-libs` two levels above the `app` folder, that is next to the source folder. The qmake output states whether the engine is on and which root was used.

The script `tools/fetch-speech-bundle.sh [directory]` does the download, the check and the unpacking: it takes the file name, URL and checksum from `tools/speech-bundle.conf` and unpacks into the directory given, by default `../build/speech-libs` next to the source folder. It does nothing if that directory already holds the bundle.

A build without the bundle, and every build for i486, has no speech engine: it builds and runs normally, and the spoken announcements are hidden.

## Documentation

- [Requirements](docs/requirements.md)
- [Architecture](docs/architecture.md)

## Privacy

The app makes no network connections. Workouts, exercises and the session log are stored in the app's own data directory. Exported files go to `~/Documents/WorkoutTrainer`.

## Development

The development of this project included generative AI tools.

## Licence

GPL-3.0-or-later. The muscle-group pictures are released under CC0 1.0. The digit font DSEG7 Classic by keshikan is licensed under the SIL Open Font License 1.1; its licence text is in `app/fonts/DSEG-LICENSE.txt` and ships with the app.

---

[01]: /Screenshots/01-first.png "First start, language choice"
[02]: /Screenshots/02-home.png "Home in-app menu"
[03]: /Screenshots/03-interval.png "Simplest interval training timer"
[04]: /Screenshots/04-metronome.png "Setting exercises tempos"
[05]: /Screenshots/05-exercises.png "Exercises list, custome exercises allowed!"
[06]: /Screenshots/06-exercise.png "Exercise file with description, tips, muscle groups. Create your own!"
[07]: /Screenshots/07-workouts.png "Workouts list, add your own!"
[08]: /Screenshots/08-session.png "How a workout session runtime looks like"
