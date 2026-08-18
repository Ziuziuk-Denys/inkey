# inkey

Offline text-augmentation IBus engine. This is a Phase 0 skeleton: it proves
the pipeline end to end (IBus captures a keystroke, hands it to a Rust core
over a C FFI boundary, commits the transformed result back into the focused
app). The transform itself is a placeholder (uppercasing) - correction,
prediction, and punctuation logic land in later phases.

## Layout

- `core/` - Rust crate (`cdylib` + `staticlib`), exposes `inkey_transform`
  and `inkey_free_string` over a C ABI. A panic inside the transform never
  crashes the caller; it falls back to returning the original text.
- `engine/` - C `IBusEngine` implementation, built with meson, linking
  against `core/`. Registers as component `org.freedesktop.IBus.Inkey`,
  engine name `inkey`.

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

1. GNOME Settings -> Keyboard -> Input Sources -> Add an Input Source ->
   find "Inkey (Phase 0)".
2. Switch to it with the input source switcher.
3. Type into GNOME Text Editor or Firefox's address bar - typed characters
   should commit uppercased.

Rebuilding after editing `core/` or `engine/` regenerates
`engine/build/inkey.xml` (meson bakes in the absolute exec path), so the
symlink above doesn't need to be recreated.
