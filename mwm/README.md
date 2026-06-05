<picture>
  <source media="(prefers-color-scheme: dark)" srcset=".github/mwm.png">
  <img alt="mwm screenshot" src=".github/mwm.png">
</picture>

# mwm — matteo's window manager

A deliberately small, dwm-flavoured tiling window manager for X11. Suckless in
spirit: written in C, configured by editing a header and recompiling, few
dependencies, little code.

The one big departure from dwm: **mwm has no built-in keybindings.** Everything
is driven at runtime through the `mwmc` client over a pure-X11 IPC channel
(tmux style). Bind keys to `mwmc` calls with any hotkey daemon (e.g. `sxhkd`).

## Features

- **Tags** — one global pool of up to 100 tags, *shared across monitors*.
- **Two tiling layouts** plus floating:
  - `|||` **cols** — equal, full-height columns.
  - `||=` **stack** — one master column + the rest stacked as rows.
  - `><>` **float**.
- **window ≤ tag ≤ monitor** model: a window lives in a tag; a tag is owned by
  one monitor. Move a window to a tag, or a whole tag (with its windows) to
  another monitor.
- **Configurable gaps** between/around windows (live-adjustable).
- **Rounded corners** for clients via the X Shape extension — works with *no
  compositor* (turn off if your compositor already rounds corners).
- **Translucent three-segment bar** (left: tags + layout, center: focused
  title, right: status), each segment a rounded "pill" with its own
  configurable background/alpha (alpha 0 = no background).
- **System tray** (freedesktop XEmbed) embedded in the bar.
- **Status from the root window name**, so **slstatus works unmodified**.
- **Font support** via Xft (with fallback fonts).
- **Window rules**: match by class/instance/title and/or `_NET_WM_WINDOW_TYPE`
  to auto-place a window on a tag (by **name** or number) and/or float it —
  including catch-all rules like "float every dialog/utility/toolbar/splash".
- **Tag→monitor rules**: assign default tags to a monitor when it appears.
- Control everything via **`mwmc`** (and query state for scripts/menus).

## Build

Dependencies: `libX11`, `libXft` (+ `fontconfig`, `freetype2`), `libXext`
(Shape), and optionally `libXinerama` (multi-monitor).

```sh
make            # builds ./mwm and ./mwmc (creates config.h from config.def.h)
sudo make install
```

Edit `config.h` and re-run `make` to change settings. To build without
Xinerama, comment out the two `XINERAMA*` lines in `config.mk`.

## Running

mwm does not spawn anything itself. Start it from `~/.xinitrc` and launch your
status bar, compositor, and hotkey daemon alongside it — see
[`examples/xinitrc`](examples/xinitrc):

```sh
slstatus &                     # writes the root name -> right bar pill
picom &                        # optional: real bar translucency
sxhkd &                        # your keybindings -> mwmc calls
exec mwm
```

## Controlling mwm — the `mwmc` grammar

Everything hangs off the containment hierarchy **window ≤ tag ≤ monitor**. Two
verbs do most of the work; a verb's target type is inferred from its subject.

```
# FOCUS — move attention
mwmc focus next|prev|last            # focus next/previous/last-used window
mwmc focus win next|prev|last|<class> # by direction, last-used, or WM_CLASS
mwmc focus tag next|prev|<name>      # view a tag (claims it for the focused monitor if unowned)
mwmc focus mon next|prev|<n>         # focus a monitor

# MOVE — relocate a subject into its container's neighbour
mwmc move win next|prev|<tag>        # send focused window to a tag
mwmc move win master                 # promote focused window to the master slot
mwmc move win mon next|prev          # send focused window to another monitor
mwmc move tag next|prev|<mon>        # move the current tag (+its windows) to a monitor
mwmc swap next|prev                  # reorder focused window within the stack

# LAYOUT & geometry (per-tag)
mwmc layout cols|stack|float|next|prev
mwmc master +1|-1|<n>                # master count
mwmc master ratio +0.05|-0.05|<f>    # master width fraction
mwmc gap +5|-5|<px>                  # window gaps

# WINDOW state & floating geometry
mwmc win toggle float|full|sticky
mwmc win tag <name>                  # (alias of `move win <tag>`)
mwmc win move <dx> <dy>              # nudge a floating window (auto-floats it)
mwmc win resize <dw> <dh>            # grow/shrink a floating window
mwmc win center                      # center a floating window
mwmc win close
# mouse: hold Super and drag = move, Super + right-drag = resize (config: movemod)

# BAR / session
mwmc bar toggle|show|hide            # show/hide the bar
mwmc traybar toggle|show|hide        # show/hide the system tray
mwmc reload | mwmc quit | mwmc version

# INTROSPECTION — for scripts, menus, and external keybinders
mwmc query tags                      # index, name, owner monitor, selected, occupied, layout
mwmc query windows                   # id, tag, class, flags, title
mwmc query monitors                  # geometry, selected tag
mwmc query layout | mwmc query state
```

`mwmc help` prints this reference. Commands return their output on stdout and a
non-zero exit code (with an `error:` line) on failure.

### How the IPC works

No sockets, no daemon, no extra dependency: `mwmc` opens its own X connection,
stashes the command on a throwaway window, sends a `ClientMessage` to the root
window, and reads mwm's reply back off that window (see `ipc.h`). Any number of
`mwmc` calls can run concurrently.

## Keybindings

There are none built in — that's the point. Map keys to `mwmc` with `sxhkd` (or
xbindkeys, or your DE's shortcuts). A full example is in
[`examples/sxhkdrc`](examples/sxhkdrc):

```
super + j            mwmc focus next
super + {1-9}        mwmc focus tag {1-9}
super + shift + {1-9} mwmc move win {1-9}
super + t            mwmc layout cols
super + shift + q    mwmc quit
```

## Tags & monitors

- There is **one global pool of tags** (named in `config.h`; unnamed tags show
  their number, up to 100).
- **Each tag is owned by exactly one monitor** and its windows render there.
- A monitor displays **one** of the tags it owns at a time. The bar shows that
  monitor's owned tags that are occupied or selected.
- **`mwmc move tag <mon>`** hands the current tag (and its windows) to another
  monitor and follows it there.
- **A newly connected monitor owns no tags** (blank) unless a `tagrules[]`
  entry in `config.h` assigns one. Claim a tag for it with
  `mwmc focus tag <name>`.

The shipped `config.h` bootstraps tag `1` on monitor `0` so you start with a
workspace; remove that `tagrules[]` entry for a fully blank start.

## Bar, rounded corners & translucency

- The bar is **clickable**: click a tag to view it, click the layout pill to
  cycle layouts (right-click to cycle backwards), and scroll over the tags to
  step through them.
- The bar is one transparent window per monitor; the three segments are drawn
  as rounded pills. Each pill's background colour and **alpha** come from the
  matching colour scheme in `config.h` — set a background alpha to `0` for a
  segment with no background.
- **Real translucency requires a compositor** (e.g. `picom`). Without one,
  alpha is effectively ignored by the X server.
- **Client rounded corners are done by mwm itself** via the X Shape extension,
  so they work with no compositor. If your compositor already rounds corners,
  set `roundcorners = 0` in `config.h` (and optionally enable picom's
  `corner-radius`). See [`examples/picom.conf`](examples/picom.conf).

## Configuration

Everything lives in `config.h` (start from `config.def.h`): fonts, colours +
alphas, gaps, border width, corner radius, bar geometry, layouts, default
nmaster/mfact, system-tray placement, window-class `rules[]`, and
tag→monitor `tagrules[]`. Recompile and restart to apply.

### Styling via Xresources (no recompile)

Colors, font, pill alphas, and geometry can be overridden at startup from the X
resource database — handy for theming without rebuilding. Put `mwm.*` resources
in `~/.Xresources` (see [`examples/Xresources`](examples/Xresources)), load them
before mwm starts, and restart mwm:

```sh
xrdb -merge ~/.Xresources && mwmc reload   # restyle live, no restart
```

`mwmc reload` re-reads the X resource DB and rebuilds fonts, colors, pill
alphas, gaps, borders and bar geometry on the fly (it validates colors/fonts
first, so a typo won't kill the running WM — it just reports an error and keeps
the old style).

```
mwm.font:          JetBrains Mono:size=11
mwm.selbg:         #7aa2f7
mwm.statusbgalpha: 0xa0
mwm.gappx:         12
mwm.cornerradius:  8
```

Recognised keys: `font`; `{norm,sel,urg,title,status}{fg,bg,border}`; `barbg`;
`{norm,sel,urg,title,status,bar}bgalpha`; `borderpx`, `gappx`, `cornerradius`,
`segradius`, `barpadx`, `barvertpad`, `barmargin`, `barvmargin`, `barseggap`,
`mfact`.
Anything not set keeps its `config.h` default. The mapping lives in the
`resources[]` table in `config.def.h` — add a line there to expose more.

**pywal / wallust:** those tools emit the generic `*.colorN` namespace, which
does **not** match mwm's `mwm.*` keys — so mwm won't pick them up automatically.
Map them in your wallust/pywal template (e.g. `mwm.selbg: {color4}`) — see
[`examples/Xresources.wallust`](examples/Xresources.wallust) — then
`xrdb -merge … && mwmc reload` (a good wallust post-run hook).

## Testing in the Vagrant VM

A libvirt VM (Arch, dual QXL output) is defined in `Vagrantfile`; provisioning
installs the build deps and test tools (`sxhkd`, `picom`, `xterm`, `dmenu`,
slstatus's deps, fonts). The project is synced to `/vagrant`.

```sh
make vmup                       # on the host: bring the VM up
make vmconnect                  # attach a viewer

# inside the VM:
cd /vagrant && make && sudo make install
echo 'exec mwm' >> ~/.xinitrc   # plus slstatus/sxhkd/picom (see examples/xinitrc)
startx
```

## Files

```
mwm.c          window manager
mwmc.c         control client
drw.{c,h}      drawing (Xft text, ARGB/alpha, rounded pills) — from dwm
util.{c,h}     die/ecalloc
ipc.h          IPC atom/protocol shared by mwm and mwmc
config.def.h   documented default configuration
examples/      xinitrc, sxhkdrc, picom.conf
```

## Screenshots

<details open>
<summary>Click to collapse</summary>

![screenshot 0](examples/screenshots/0.png)
*Default setup — cols layout with tags, bar, and gap.*

![screenshot 1](examples/screenshots/1.png)
*Stack layout with floating window.*

</details>

## License

MIT Derived in part from dwm (suckless.org).
