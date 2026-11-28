# Vibeyrino (extended notes)

See [README.md](README.md) for the GitHub overview, disclaimer, credits, and feature list.

## Philosophy

This is a **personal, vibecoded** fork. I build what I need for my own machine. **No support expectations**—use it, fork it, or lift individual commits freely (MIT).

## Native Qt patch

UI tweaks from [hekel/chatterino-stuffs](https://github.com/hekel/chatterino-stuffs) (native-style settings) live in [`patches/native-qt/`](patches/native-qt/):

- `nativeQT.patch` — settings QSS / widget styling
- `fix-bg.patch`, `fix-svg.patch` — follow-up fixes

On Linux, the app is meant to blend with the desktop Qt theme rather than shipping a hard-coded Fusion look.

## Quick settings map

| Area | Where |
| ---- | ----- |
| Gruvbox themes | Appearance → Theme → **Gruvbox Soft** / **Gruvbox Material** |
| Kick defer + Centrifugo | Settings → **General** (Kick section) |
| YouTube badges, Super Chats, colors | Settings → **Leafyrino** |
| Live-only `/live` tabs, YouTube waiting rooms, tab YouTube refresh behavior | Settings → **Vibeyrino** |
| Streamlink fallback and source priority | Settings → **External Tools**; per-tab overrides in **Right-click → Change channel → Multi** |

## Build & install

Same as README:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo install -m 755 build/bin/Vibeyrino /usr/bin/Vibeyrino
```

Optional icons (also keeps compatibility with older **Leafyrino** icon names):

```bash
sudo install -Dm644 resources/com.chatterino.chatterino.desktop \
  /usr/share/applications/com.vibeyrino.desktop
sudo install -Dm644 resources/icon-vibeyrino.png /usr/share/pixmaps/com.vibeyrino.png
sudo install -Dm644 resources/icon-vibeyrino.png /usr/share/icons/hicolor/256x256/apps/com.vibeyrino.png
sudo install -Dm644 resources/icon-vibeyrino.png /usr/share/icons/hicolor/256x256/apps/com.leafyzito.leafyrino.png
```

Quit Leafyrino/Vibeyrino before launching a newly installed binary.