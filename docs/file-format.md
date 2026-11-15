# File formats

Workout Trainer reads and writes three kinds of JSON file: the workout export file (R-IMP-1), the starter library file (R-EXR-5) and the starter workouts file (R-TPL-2). Both are plain UTF-8 text. This page describes them so that other tools can read and write them (R-IMP-3).

## Common rules

- Encoding: UTF-8. A leading byte order mark is accepted and ignored.
- Every file is one JSON object with a `format` text and a `version` whole number. A file with another `format` is rejected. The workout export file is at version 4 and the starter library file and the starter workouts file are at version 1. Until the first release the app reads exactly one version of each: a file with another `version` is rejected with the reason that its version is newer than the app knows or no longer supported, and nothing is changed.
- Unknown extra fields are ignored, so a later version can add fields without breaking this one.
- Whole numbers must be written without a fraction or exponent that makes them fractional (`30`, not `30.5`).
- Texts count UTF-16 code units, like the limits in the app. Names are trimmed of leading and trailing white space.
- Limits of a file: at most 4 MiB, 200 workouts, 1000 exercises and 1000 metronomes.
- An import checks the whole file first. If anything is invalid, the file is rejected and nothing is changed. The error list names each place, for example `workouts[2].blocks[0].steps[1].reps`, and never repeats your own text.

## Export file

```
{
    "format": "workout-trainer-workouts",
    "version": 3,
    "exercises": [ exercise, ... ],
    "metronomes": [ metronome, ... ],
    "workouts": [ workout, ... ]
}
```

`exercises` and `workouts` are required. `metronomes` is written by the app and may be absent. The app writes the keys in alphabetical order with four-space indentation and a final newline; readers must not depend on the order.

The file holds only the exercises that the exported workouts use and the metronomes that those exercises name. Workout ids, the favorite flag and the starter mark of an exercise are not exported.

### Exercise

| Field | Type | Required | Rule |
|---|---|---|---|
| `key` | text | yes | 1 to 64 characters: ASCII letters, digits, `-` and `_`. Unique in the file. |
| `name` | text | yes | 1 to 80 characters after trimming. Names need not be unique. |
| `muscles` | list of texts | yes | Muscle group keys, at least one. |
| `instructions` | text | no | At most 4000 characters. Empty when absent. |
| `tips` | text | no | At most 2000 characters. Empty when absent. |
| `icon` | text | no | 1 to 40 lower-case letters, digits and hyphens. Omitted when the exercise has no icon. |
| `colour` | text | no | One of the colour keys below. Omitted when the exercise has the default colour. |
| `language` | text | no | Language tag of the texts (R-EXR-12): 2 to 35 lower-case letters, digits and hyphens, starting with a letter, for example `en` or `pt-br`. The app always writes it. `en` when absent. |
| `metronome` | text | no | Key of the metronome that gives the exercise its tempo (R-EXR-10): a metronome of the file, or one the app already has. The metronome is a rhythm: the phases of one repetition. Omitted when the exercise has no metronome. |

Muscle group keys (R-MUS-4), in the order the app writes them: `shoulders`, `core` (the upper abs), `glutes`, `quadriceps`, `hamstrings`, `calves`, `lower-abdomen`, `lower-back`, `upper-chest`, `lower-chest`, `obliques`, `trapezius`, `lats`, `mid-back`, `biceps`, `triceps`, `forearms`, `shins`. These are the eighteen active groups.

The keys `chest`, `back` and `arms` are older names. They are still accepted on import and are read as the groups that replaced them: `chest` as `upper-chest` and `lower-chest`; `back` as `trapezius`, `lats` and `mid-back`; `arms` as `biceps`, `triceps` and `forearms`. The app never writes them, and an import does not report them. Any other key is unknown (see below).

Colour keys (R-EXR-8): `red`, `orange`, `yellow`, `green`, `teal`, `blue`, `purple`, `pink`, `brown`, `grey`. The file names a colour by key; the app chooses the actual colour.

The language tag is not checked against a list of languages. An exercise without a tag is read as English.

The icon key is not checked against the pictures of the app. An icon key from a newer version is kept, and the app shows its default picture for a key it does not know.

### Workout

| Field | Type | Required | Rule |
|---|---|---|---|
| `name` | text | yes | 1 to 80 characters after trimming. |
| `restBetweenBlocksSec` | whole number | no | 0 to 600 seconds. 0 when absent. |
| `icon` | text | no | 1 to 40 lower-case letters, digits and hyphens. Omitted when the workout has no icon. |
| `colour` | text | no | One of the colour keys above. Omitted when the workout has the default colour. |
| `blocks` | list | yes | 1 to 50 blocks. |

The icon and the colour of a workout follow the rules of the exercise fields of the same name. Files written before these fields existed have none and are read as workouts with the default icon and colour; the format version stays 1, because the fields are optional and unknown fields were always ignored. Challenges of the time attack mode are not part of the file.

### Block

| Field | Type | Required | Rule |
|---|---|---|---|
| `rounds` | whole number | no | 1 to 99. 1 when absent. |
| `restBetweenRoundsSec` | whole number | no | 0 to 600 seconds. 0 when absent. |
| `steps` | list | yes | 1 to 50 steps. |

### Step

| Field | Type | Required | Rule |
|---|---|---|---|
| `type` | text | yes | `work`, `rest`, `warmup` or `cooldown`. |
| `exercise` | text | for all types except `rest` | Key of an exercise of the file, or of an exercise the app already has. A `rest` step must not have it. |
| `durationSec` | whole number | one of the two | 1 to 3600 seconds. |
| `reps` | whole number | one of the two | 1 to 999 repetitions. Not allowed for a `rest` step. |
| `startOnTap` | true or false | no | `true` if the session waits before this step until the user taps to start it. `false` when absent. The app writes it only when it is `true`. The first step of the first block always starts automatically: a `true` there is read as `false`, without a message. |

| `pace` | object | no | Only with `reps`. How the repetitions are timed when the exercise has no rhythm: `form` is `total` (the repetitions are spread evenly over `ms` milliseconds) or `perRep` (one repetition lasts `ms` milliseconds); `ms` is a whole number. One repetition, taken as the total time divided by the repetitions or as the given time, lasts 0.5 s to 60 s. One beat sounds at the start of each repetition. Without `pace` the repetitions end on a tap on "done". |

A step has either `durationSec` or `reps`, never both. A `pace` on a step that has `durationSec` is invalid. A step that ends by repetitions and whose exercise has a rhythm runs its own repetitions at the rhythm: the phases of the rhythm decide the pace, and a `pace` of the step is kept but not used while the exercise has a rhythm. A step that ends by duration ignores the rhythm. A step has no field for a metronome.

### Metronome

A saved metronome, shown to the user as a rhythm (the `metronomes` list). Several exercises can name the same metronome by its key. A rhythm is only the phases of one repetition; repetitions, sets and rest belong to what uses it. A rhythm has no icon and no colour; an `icon` or `colour` field found on a rhythm in a file written by an earlier version is read and ignored.

| Field | Type | Required | Rule |
|---|---|---|---|
| `key` | text | yes | 1 to 64 characters: ASCII letters, digits, `-` and `_`. Unique in the file. The app writes `m-` and 32 hexadecimal digits. |
| `name` | text | yes | 1 to 80 characters after trimming. |
| `phases` | list of whole numbers | yes | 1 to 8 phases of one repetition, each in half seconds, 1 to 20 (0.5 s to 10 s). A beat sounds at the start of each phase. The phases of one repetition add up to 0.5 s to 60 s. |
| `phaseNames` | list of texts | no | One text per phase, each of at most 12 characters, an empty text meaning no name. Written only when at least one phase has a name; a list of another length is refused. |

A file with a missing or mistyped field, a value outside these ranges, or an exercise that names a metronome that neither the file nor the app has is invalid, with the location of the offending value, for example `exercises[3].metronome` or `metronomes[1].phases`.

## What an import does

The import has two phases. The first reads and checks the file and reports what would happen; it changes nothing. The second applies the result in one step: either everything is stored or nothing is.

- **Workouts** are always added as new workouts. A workout is identified by its name together with its icon key and colour key; names are compared without regard to case and without surrounding blank space, and an empty key is the default. A workout with the same name, icon and colour as an existing one gets the suffix ` (imported)`, then ` (imported 2)` and so on, until its identity is free. The same name with another icon or colour is imported unchanged. Imported workouts are not favorites.
- **Exercises with a new key** are added. If the name equals the name of an existing exercise, the exercise is still added as a separate one, and the app mentions it; names need not be unique.
- **Exercises with a known key and identical content** are reused without a message.
- **Exercises with a known key and different content** are a conflict. Nothing is overwritten unless you choose it, and for each conflict the app lists the workouts that use the existing exercise (R-EXR-10). The choices are: keep the existing exercise (the default), replace it with the content of the file, or import the file's exercise as a new one with a new key and the name extended with ` (imported)`, ` (imported 2)` and so on.
- **Unknown muscle group keys** (R-MUS-5): if an exercise has known and unknown keys, the unknown ones are dropped and the app tells you. If all its keys are unknown, the exercise cannot be stored: it is skipped together with every workout that uses it, and the app tells you where. The rest of the file is still imported.
- **Unknown colour keys**, on exercises and on workouts, are replaced by the default colour and the app tells you.
- **Metronomes** are added before the exercises that name them. A metronome with a key the app already has and the same content (name, phases and phase names) is the same record: it is not added again, and the exercises link the one the app has. A metronome with a known key and other content is added as a new metronome with a new key, so the one the app has stays as it is. A new metronome whose name equals that of another metronome (compared without regard to case and to surrounding blank space) gets the suffix ` (imported)`, then ` (imported 2)` and so on, until its identity is free. An exercise may also name the key of a metronome the app has that the file does not carry.
- **The metronome of an exercise is part of its content** in the file. An exercise with a known key that is equal in every field but names another metronome, or none while the app's exercise has one (or the other way round), is a conflict with the choices above: keeping leaves the app's exercise and its metronome, replacing takes the file's metronome (or none), importing as new adds the exercise with the file's metronome.
- **Start mode** is read as part of the step. A step without it starts automatically.
- **Invalid icon keys** (wrong characters or too long), on exercises and on workouts, make the file invalid. A well-formed icon key that the app does not know is kept.
- **Language tags** that are not well formed make the file invalid. A well-formed tag that the app does not know is kept. Two exercises with the same key and a different language have different content, so they are a conflict.
- Imported exercises never carry the starter mark.

## Starter library files

The starter library (R-EXR-5, R-EXR-15) is a structure file that does not depend on a language, one texts file for each shipped language, and a file of workouts. All are installed with the app and share the rules of the export file: JSON, UTF-8 (the files of the project are written with ASCII escapes), values checked against the same ranges, a size limit of 4 MiB each.

Identifiers are 8 lower-case hexadecimal digits (`0000002f`), one sequence per kind (exercise, rhythm, workout), unique in the file, assigned once and never changed.

`exercises.json` (the structure):

```
{
    "format": "workout-trainer-starter",
    "version": 1,
    "exercises": [ { "id", "colour", "icon", "muscles": [..], "metronome": <rhythm id, optional> }, ... ],
    "metronomes": [ { "id", "phases": [..], "phaseNames" optional }, ... ]
}
```

`texts.<language>.json` (for example `texts.en.json`, `texts.it.json`):

```
{
    "format": "workout-trainer-starter-texts",
    "version": 1,
    "language": "it",
    "exercises": { "<id>": { "name", "instructions", "tips" }, ... },
    "metronomes": { "<id>": { "name", "phaseNames": [..] }, ... },
    "workouts": { "<id>": { "name", "blocks": [ block name, ... ] }, ... }
}
```

English is complete and is the fallback for a missing text. A text key that is not an identifier of the structure is ignored and reported on the log; an entry without an English text is an error.

Offering and language: the entries of the structure are added once per identifier, in the language the user has chosen (their texts and tag come from that language's file); the table of offered identifiers is shared by exercises, rhythms and workouts of the same kind. An entry that was offered before is never added again, even if the user deleted or changed it. Entries of the library that still follow the language (not edited, R-EXR-15) take the texts and the tag of the new language when the language is changed. The first edit of a text of an entry makes it keep its texts. A rhythm is offered before the exercises that name it; an exercise whose rhythm is missing is added without its rhythm and the problem is logged. An exercise whose muscle group keys are all unknown to this version is skipped. The structure and the texts file are checked completely before anything is added; a file with a problem adds nothing and the reason is logged.

## Starter workouts file

`workouts.json` has the same rules, with `"format": "workout-trainer-starter-workouts"`, `"version": 1` and a `workouts` list of the workout objects of the export file, each with an `id` (see above) instead of a name, and `name`, block names in the texts files (R-TPL-2, for example 20 s of work and 10 s of rest in 8 rounds). A step refers to an exercise by the identifier of a starter exercise or by the key of any exercise the app has, and may carry the optional `pace` of the export file. The file is read after the library; a workout with a problem (a repeated id, a value out of range, an exercise that neither the library nor the app has) is left out and logged, the others are loaded, and the one left out is offered again at the next start. A loaded workout is an ordinary workout: it can be edited, duplicated and deleted, it is not a favorite, and its first step starts automatically. A workout whose name, icon and colour equal those of a workout the user has gets the suffix ` (starter)`, then ` (starter 2)`, and so on.

## Example

```json
{
    "exercises": [
        {
            "colour": "blue",
            "icon": "squat",
            "instructions": "Stand with your feet shoulder-width apart. Lower your hips, then stand up.",
            "key": "u-example-squat",
            "language": "en",
            "metronome": "m-example-slow",
            "muscles": [
                "glutes",
                "quadriceps"
            ],
            "name": "Squat",
            "tips": "Keep your heels on the floor."
        },
        {
            "instructions": "",
            "key": "u-example-plank",
            "language": "en",
            "muscles": [
                "core"
            ],
            "name": "Plank",
            "tips": ""
        }
    ],
    "format": "workout-trainer-workouts",
    "metronomes": [
        {
            "key": "m-example-slow",
            "name": "Slow squat",
            "phases": [
                4,
                2,
                4
            ]
        }
    ],
    "version": 4,
    "workouts": [
        {
            "blocks": [
                {
                    "restBetweenRoundsSec": 15,
                    "rounds": 3,
                    "steps": [
                        {
                            "durationSec": 40,
                            "exercise": "u-example-squat",
                            "type": "work"
                        },
                        {
                            "durationSec": 10,
                            "type": "rest"
                        },
                        {
                            "durationSec": 30,
                            "exercise": "u-example-plank",
                            "type": "work"
                        }
                    ]
                },
                {
                    "restBetweenRoundsSec": 0,
                    "rounds": 1,
                    "steps": [
                        {
                            "exercise": "u-example-squat",
                            "reps": 12,
                            "type": "cooldown"
                        },
                        {
                            "exercise": "u-example-plank",
                            "pace": {
                                "form": "total",
                                "ms": 30000
                            },
                            "reps": 10,
                            "type": "work"
                        }
                    ]
                }
            ],
            "name": "Example circuit",
            "restBetweenBlocksSec": 30
        }
    ]
}
```
