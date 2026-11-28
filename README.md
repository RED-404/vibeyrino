# Vibeyrino

**Personal, vibecoded Chatterino fork.** Built for my own setup—not a supported product.

There is **no promise of maintenance, fixes, or replies** to issues or PRs. If something here is useful, **take any patch or idea you want** (MIT license applies; see [LICENSE](LICENSE)).

## Credits & lineage

Vibeyrino sits on top of a lot of community work. Thank you to:

| Project | Role |
| --------| ----- |
| [Chatterino 2](https://github.com/Chatterino/chatterino2) | Original client |
| [Chatterino7](https://github.com/SevenTV/chatterino7) | 7TV and related Chatterino2 extensions |
| [Leafyrino](https://github.com/Leafyzito/leafyrino) | Direct base fork (Kick/YouTube multistream, badges, Leafyrino settings, and more) |
| [Native Qt settings patches](https://github.com/hekel/chatterino-stuffs) ([`chatterino2-7tv-native-git`](https://github.com/hekel/chatterino-stuffs)) | Lighter settings UI that follows the desktop/Qt style; vendored under [`patches/native-qt/`](patches/native-qt/) |

Leafyrino itself pulls ideas from forks like [Technorino](https://github.com/2547techno/technorino/) and [Moltorino7](https://github.com/EEVA-inc/Moltorino7). Vibeyrino keeps that stack and adds the changes below.

## What Vibeyrino adds (on top of Leafyrino)

### App & UI

- **Rebrand** to Vibeyrino (binary name, icon, window titles, tray).
- **Settings → Vibeyrino** — fork-only options (see below).
- **Gruvbox Soft** and **Gruvbox Material** built-in themes (`resources/themes/`).
- **Native Qt appearance on Linux** — system Qt style instead of a forced Fusion palette; settings dialogs use trimmed QSS on top of the desktop palette ([`patches/native-qt/`](patches/native-qt/)).

### Notebook & tabs

- **Always show `/live`, `/mentions`, `/watching`, `/whispers`, `/automod` in live-only tab mode** (Settings → Vibeyrino → Notebook).

### Kick

- **Kick connection preference** — Pusher, Centrifugo, or Both (Settings → General; restart required).
- **Defer Kick live chat until the channel is live** — reduces WebSocket load with many tabs.
- **…or connect while the Kick split is selected** — still defer globally, but join chat when you focus that split (refreshes live status on focus).

### YouTube

- **Live vs scheduled vs offline** — uses YouTube `playabilityStatus` so waiting rooms are not treated as live unless you opt in.
- **Connect to scheduled waiting rooms** (Settings → Vibeyrino) — join premiere / waiting-room chat; shown as **(scheduled)**; does not count as live for tab filters.
- **Auto YouTube refresh** when you select a **live** tab that still has **offline** YouTube (including multichannel merges).
- Leafyrino’s YouTube chat UX (Super Chats, role colors, header refresh button, etc.) remains under **Settings → Leafyrino**.

### Streamlink

- **Source fallback for multichannel splits** — opening a multichannel split with Streamlink tries the next configured source if the preferred one is unavailable.
- **Global sortable source priority** — **Settings → External Tools** provides **Try fallback sources** and a drag-sortable platform priority list.
- **Per-tab Streamlink overrides** — **Right-click → Change channel → Multi** provides an independent fallback checkbox and drag-sortable source priority for that split. These overrides do not change global settings.

## Build & install

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo install -m 755 build/bin/Vibeyrino /usr/bin/Vibeyrino
```

Optional Linux desktop entry:

```bash
sudo install -Dm644 resources/com.chatterino.chatterino.desktop \
  /usr/share/applications/com.vibeyrino.desktop
sudo install -Dm644 resources/icon-vibeyrino.png /usr/share/pixmaps/com.vibeyrino.png
```

Quit any running Leafyrino/Vibeyrino before replacing the binary.

### Arch Linux (`PKGBUILD`)

This repo includes a **`vibeyrino-git`** [PKGBUILD](PKGBUILD) (same layout as [Leafyrino’s AUR package](https://aur.archlinux.org/packages/leafyrino-git)).

1. Clone from [`github.com/RED-404/vibeyrino`](https://github.com/RED-404/vibeyrino) (default branch `leafyrino`).
2. Install build deps, e.g. `sudo pacman -S --needed base-devel git cmake ninja boost rapidjson hunspell libavif libnotify qt6-{base,charts,imageformats,svg} qtkeychain-qt6`
3. `makepkg -si`

For a local checkout without pushing: `makepkg --noextract`, replace `src/vibeyrino` with your tree or use `source=("git+file:///path/to/vibeyrino#branch=leafyrino")` in a copy of the PKGBUILD.

More notes: [VIBERYRINO.md](VIBERYRINO.md).

## Settings & data

Config and layouts still use the usual Chatterino data dir (e.g. `~/.local/share/chatterino/`), so existing accounts and layouts from Leafyrino/Chatterino generally carry over.

## Upstream

An `upstream` remote may point at [Leafyrino](https://github.com/Leafyzito/leafyrino) for reference when merging or diffing. This repo is not meant to replace or compete with Leafyrino or Chatterino7—it's a personal snapshot.