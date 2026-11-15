# Bundled sounds

All sounds of the app are original. They are synthesised from the parameters below by
`tools/make-sounds.py` (Python standard library only, no recorded or third-party material, no
speech). Run `tools/make-sounds.py` to write the files again and `tools/make-sounds.py --check` to
verify that the files in this folder equal the synthesis of the parameters.

- Licence of every file: CC0 1.0 Universal (public domain dedication).
- Provenance of every file: synthesised by `tools/make-sounds.py`.
- Format: WAV, PCM, mono, 16 bit, 44100 Hz. Peak -3 dBFS. A 2 ms raised-cosine fade at the start and
  at the end of each file.
- The app plays only these files; it accepts no user-supplied audio (R-ALR-5).

- `click-first.wav`
  - Event: Metronome click, first beat of a repetition
  - Parameters: 1320 Hz sine + 2nd harmonic 0.25, exponential decay 12 ms
  - Length: 0.070 s
- `click-middle.wav`
  - Event: Metronome click, beat between the first and the last
  - Parameters: 660 Hz sine + 2nd harmonic 0.25, exponential decay 12 ms
  - Length: 0.070 s
- `click-last.wav`
  - Event: Metronome click, last beat of a repetition
  - Parameters: 990 Hz sine + 2nd harmonic 0.25, exponential decay 12 ms
  - Length: 0.070 s
- `lead-in-tick.wav`
  - Event: Tick of the get-ready countdown before a step
  - Parameters: 800 Hz sine, raised-cosine envelope (no attack edge)
  - Length: 0.050 s
- `tick.wav`
  - Event: Tick at the listed last seconds of a period, a countdown or a best-time bar
  - Parameters: 1760 Hz sine + 3rd harmonic 0.2, flat envelope, 6 ms attack, 20 ms release
  - Length: 0.090 s
- `work-start.wav`
  - Event: A work, warm-up or cool-down step starts; a stopwatch or time attack run starts
  - Parameters: rising glide 600 to 900 Hz + 3rd harmonic 0.15, flat envelope, 10 ms attack, 80 ms release
  - Length: 0.300 s
- `rest-start.wav`
  - Event: A rest period starts
  - Parameters: falling glide 700 to 440 Hz + 2nd harmonic 0.2, 30 ms attack, exponential decay 220 ms
  - Length: 0.420 s
- `round-change.wav`
  - Event: A new round or set begins
  - Parameters: notes 700, 700, 1050 Hz (+ harmonics 0.3, 0.1) of 90, 90, 140 ms with 40 ms gaps
  - Length: 0.400 s
- `wait-for-tap.wav`
  - Event: A step waits for the tap that starts it; repeated every 3 s until the tap
  - Parameters: two descending notes 988 and 784 Hz (+ harmonics 0.3, 0.1) of 180 and 280 ms with a 30 ms gap, bell envelope (4 ms attack, decay 90 ms and 130 ms)
  - Length: 0.490 s
- `timer-end.wav`
  - Event: A countdown reaches zero; a stopwatch or time attack run stops
  - Parameters: two beeps 1000 Hz + harmonics 0.3, 0.1, flat envelope, 160 ms each, 80 ms gap
  - Length: 0.400 s
- `workout-complete.wav`
  - Event: The workout, a metronome run or a session is complete
  - Parameters: rising C major arpeggio 523.25, 659.25, 783.99, 1046.50 Hz (+ harmonics 0.3, 0.1) every 180 ms, bell envelope
  - Length: 1.140 s
