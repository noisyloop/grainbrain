#!/usr/bin/env bash
set -euo pipefail
# Install a trusted, locally built GrainBrain release without notarization.
# Usage: bash install-macos.sh /path/to/extracted/GrainBrain-macOS
SOURCE="${1:-.}"
SOURCE="$(cd "$SOURCE" && pwd)"
find_bundle() { find "$SOURCE" -type d -name "$1" -print -quit; }
APP="$(find_bundle GrainBrain.app)"
VST="$(find_bundle GrainBrain.vst3)"
AU="$(find_bundle GrainBrain.component)"
if [[ -z "$APP" && -z "$VST" && -z "$AU" ]]; then
  echo "No GrainBrain bundles found in $SOURCE" >&2
  exit 1
fi
if [[ -n "$APP" ]]; then
  echo "Installing standalone app..."
  sudo ditto "$APP" /Applications/GrainBrain.app
  sudo xattr -dr com.apple.quarantine /Applications/GrainBrain.app || true
fi
if [[ -n "$VST" ]]; then
  echo "Installing VST3..."
  mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
  ditto "$VST" "$HOME/Library/Audio/Plug-Ins/VST3/GrainBrain.vst3"
  xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/GrainBrain.vst3" || true
fi
if [[ -n "$AU" ]]; then
  echo "Installing Audio Unit..."
  mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
  ditto "$AU" "$HOME/Library/Audio/Plug-Ins/Components/GrainBrain.component"
  xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/GrainBrain.component" || true
fi
echo "Installation complete. Restart your DAW and rescan plugins if needed."
