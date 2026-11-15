#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Michele Brami
#
# Downloads the speech bundle named in tools/speech-bundle.conf, checks its SHA-256 and unpacks it
# into the directory (default ../build/speech-libs), which then works as the speech root
# (RT_SPEECH_ROOT) of a build. Nothing happens if that directory already holds this bundle.
#
# usage: tools/fetch-speech-bundle.sh [directory]
# The environment variable SPEECH_BUNDLE_CONF selects another conf file.
# Exit status: 0 ready, 2 usage or conf error, 3 download failed, 4 checksum mismatch,
#              5 the directory holds something else, 6 unpacking failed.
set -uo pipefail
SRC="$(cd "$(dirname "$0")/.." && pwd)"
CONF="${SPEECH_BUNDLE_CONF:-$SRC/tools/speech-bundle.conf}"
[ -f "$CONF" ] || { echo "no conf file: $CONF" >&2; exit 2; }
source "$CONF"
for v in SPEECH_BUNDLE_NUMBER SPEECH_BUNDLE_FILE SPEECH_BUNDLE_URL SPEECH_BUNDLE_SHA256; do
    [ -n "${!v:-}" ] || { echo "$v is not set in $CONF" >&2; exit 2; }
done
DIR="${1:-$SRC/../build/speech-libs}"
mkdir -p "$(dirname "$DIR")" || exit 2
DIR="$(cd "$(dirname "$DIR")" && pwd)/$(basename "$DIR")"
MARK="speech-bundle $SPEECH_BUNDLE_NUMBER $SPEECH_BUNDLE_SHA256"

if [ -f "$DIR/.speech-bundle" ] && [ "$(cat "$DIR/.speech-bundle")" = "$MARK" ] \
        && [ -f "$DIR/MANIFEST.txt" ] && [ -d "$DIR/aarch64/lib" ]; then
    echo "Speech bundle $SPEECH_BUNDLE_NUMBER is already in $DIR"
    exit 0
fi
if [ -e "$DIR" ] && [ -n "$(ls -A "$DIR" 2>/dev/null)" ] && [ ! -f "$DIR/.speech-bundle" ]; then
    echo "$DIR exists and was not made by this script: use another directory or remove it" >&2
    exit 5
fi

TMP="$(mktemp -d "$DIR.fetch.XXXXXX")" || { echo "cannot create a work directory next to $DIR" >&2; exit 2; }
trap 'rm -rf "$TMP"' EXIT
echo "Downloading $SPEECH_BUNDLE_URL"
if ! curl -fL --retry 3 -sS -o "$TMP/$SPEECH_BUNDLE_FILE" "$SPEECH_BUNDLE_URL"; then
    echo "download failed: $SPEECH_BUNDLE_URL" >&2
    exit 3
fi
ACTUAL="$(sha256sum "$TMP/$SPEECH_BUNDLE_FILE" | cut -d' ' -f1)"
if [ "$ACTUAL" != "$SPEECH_BUNDLE_SHA256" ]; then
    rm -f "$TMP/$SPEECH_BUNDLE_FILE"
    echo "checksum mismatch: expected $SPEECH_BUNDLE_SHA256, got $ACTUAL; the download was deleted" >&2
    exit 4
fi
mkdir "$TMP/unpacked"
if ! tar -xJf "$TMP/$SPEECH_BUNDLE_FILE" -C "$TMP/unpacked"; then
    echo "unpacking failed" >&2
    exit 6
fi
if [ ! -f "$TMP/unpacked/MANIFEST.txt" ] || [ ! -d "$TMP/unpacked/aarch64/lib" ]; then
    echo "the bundle has no manifest or no aarch64 libraries" >&2
    exit 6
fi
echo "$MARK" > "$TMP/unpacked/.speech-bundle"
rm -rf "$DIR"
mv "$TMP/unpacked" "$DIR" || { echo "cannot move the bundle into $DIR" >&2; exit 6; }
echo "Speech bundle $SPEECH_BUNDLE_NUMBER unpacked into $DIR"
