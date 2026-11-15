# Contributing

Thank you for your interest. Workout Trainer is a small volunteer project; issues and pull requests are welcome.

## Licence

The project is released under GPL-3.0-or-later. By submitting a contribution you agree that it is licensed under the same terms. Contributions must be your own work, or come from material under a GPL-3.0-or-later compatible licence with the origin stated in the pull request. Do not copy code, text or pictures from projects with an incompatible licence.

## Reporting problems

- **Bugs:** open an issue with the app version, the Sailfish OS version, the device and the steps to reproduce.
- **Security problems:** see [SECURITY.md](SECURITY.md); do not use a public issue.
- **Exercises:** the starter library is written for this project. Do not paste instructions from other sources.

## Building and testing

See the [README](README.md) for the build. Before opening a pull request:

- build with `CONFIG+=hostbuild` and run `make check`; every test must pass;
- add or update a test for the behavior you change;
- if the change touches sound, vibration, the display or the file locations, say on which device you tried it.

## Guidelines

- **Language:** code, comments, identifiers, test data and documents are in English. User-visible strings go through `qsTr()` / `tr()`, are complete sentences or labels (never assembled from fragments) and use plural forms where a number is shown.
- **Comments:** self-explanatory. State the fact or the rule; do not point to other documents. The only cross-reference allowed is a requirement number (`R-TMR-7`, `R-IMP-2`, ...) defined in [docs/requirements.md](docs/requirements.md).
- **Platform:** the app is meant to pass the Harbour validation. Do not add a QML import, a library or a permission without running `sfdk check` on the package for both targets.
- **Privacy:** no network code, no telemetry, no personal data in logs.
- **Pictures and sounds:** only original work or material under a compatible licence, with the origin stated.
- **Commits:** one logical change per commit, with a short message in the imperative.

## AI-generated code

Code written with the help of AI tools is accepted when all of the following hold:

- it is specific to an issue that was already discussed;
- it is clearly commented and explained: what it does, why, and how it was verified;
- it is thoroughly tested, on the host and, where the change touches sound, display, vibration or files, on a real device.

## Decisions and forks

The author decides what is merged, and those decisions are final in this repository. You are free to fork the repository and do what you want with your copy, as long as you respect the licence.
