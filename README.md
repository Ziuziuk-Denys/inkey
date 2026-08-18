# inkey

Offline text-augmentation IBus engine. Phase 1: word-boundary EN <-> RU/UK
wrong-layout correction. Ctrl/Alt/Super-held key combos always pass through
untouched. Plain characters are buffered per word and only inspected at a
word boundary (space, `.,!?;:`, or Enter) - if the buffered word looks like
Cyrillic typed on the wrong physical layout (scored against word-frequency
data), the already-committed original is erased and the corrected word is
committed instead; otherwise nothing changes. No preedit, no prediction, no
punctuation restoration yet - those are separate, later phases.

Inkey pins the physical XKB layout to `us` for as long as it's the active
input source (see `layout` in `engine/data/inkey.xml.in`), so it never
needs to touch `gsettings`/system layout state itself - see
`core/src/detect.rs` for the reasoning.

## Layout

- `core/` - Rust crate (`cdylib` + `staticlib`), exposes `inkey_transform`
  and `inkey_free_string` over a C ABI. `inkey_transform` runs word-boundary
  correction (`core/src/detect.rs`) using positional remap tables derived
  from system XKB data (`core/src/layout_tables.rs`) and embedded
  word-frequency data (`core/src/frequency.rs`). A panic inside the
  transform never crashes the caller; it falls back to returning the
  original text.
- `engine/` - C `IBusEngine` implementation, built with meson, linking
  against `core/`. Registers as component `org.freedesktop.IBus.Inkey`,
  engine name `inkey`. Buffers characters per word in `engine.c` and calls
  `inkey_transform` once per word boundary, not per keystroke.

## Build

Requires: `cargo`, `meson`, `ninja`, a C compiler, and `ibus-devel` /
`glib2-devel` (on Fedora: `sudo dnf install ibus-devel glib2-devel`).

```sh
# 1. Build the Rust core
cd core
cargo build --release
cd ..

# 2. Build the C engine (links against core/target/release)
cd engine
meson setup build
meson compile -C build
cd ..
```

## Register locally

IBus discovers engines via a component XML file in its component search
path. On this system that's `/usr/share/ibus/component/` by default (check
`man ibus-daemon` for `IBUS_COMPONENT_PATH` if you'd rather use a
non-default directory).

```sh
sudo ln -sf "$(pwd)/engine/build/inkey.xml" /usr/share/ibus/component/inkey.xml
ibus restart
ibus list-engine | grep -F "Inkey (Phase 0)"
```

If it doesn't show up, refresh the registry cache and check again:

```sh
ibus write-cache
ibus list-engine | grep -F "Inkey (Phase 0)"
```

If it's still missing, log out and back in (GNOME sometimes needs a fresh
session before a newly registered engine appears in Settings), then check
`journalctl --user -u org.freedesktop.IBus.session.GNOME.service`.

## Test

Automated:

```sh
cd core && cargo test && cd ..
cd engine && meson test -C build --print-errorlogs && cd ..
```

Manual (needs a real GNOME session):

1. Switch to Inkey (via GNOME Settings -> Keyboard -> Input Sources, or
   `ibus engine inkey`).
2. In GNOME Text Editor, confirm Ctrl+C/Ctrl+V still work normally.
3. Type `ghbdtn ` (with a trailing space) - it should become `привет `.
4. Type a correctly-typed English word like `hello ` - it should stay
   `hello `, unmangled.

Rebuilding after editing `core/` or `engine/` regenerates
`engine/build/inkey.xml` (meson bakes in the absolute exec path), so the
symlink above doesn't need to be recreated.
