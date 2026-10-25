#!/bin/bash
set -euo pipefail

# GrainBrain local development installer. Runs on the user's Mac, never in GitHub Actions.
SOURCE="$(cd "$(dirname "$0")" && pwd)"
echo "GrainBrain macOS Installer"
echo "Source: $SOURCE"
echo ""
echo "Quit Ableton Live and other audio hosts before continuing."
echo "Press Return to install VST3, AU, and standalone app to your user account."
read -r _

install_bundle() {
  local name="$1" destination="$2"
  if [[ -d "$SOURCE/$name" ]]; then
    mkdir -p "$(dirname "$destination")"
    if [[ -e "$destination" ]]; then
      rm -rf "$destination"
    fi
    ditto "$SOURCE/$name" "$destination"
    # This is only suitable for your own trusted, unsigned development builds.
    xattr -dr com.apple.quarantine "$destination" 2>/dev/null || true
    echo "Installed $name -> $destination"
  else
    echo "Not found in DMG: $name"
  fi
}

install_bundle "GrainBrain.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/GrainBrain.vst3"
install_bundle "GrainBrain.component" "$HOME/Library/Audio/Plug-Ins/Components/GrainBrain.component"
install_bundle "GrainBrain.app" "$HOME/Applications/GrainBrain.app"

echo ""
echo "Finished. Restart Ableton Live, enable VST3 system folders, then Option-click Rescan."
echo "Press Return to exit."
read -r _
