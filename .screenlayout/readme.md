Screen Layout
===

## Naming convention

`{layout-name}-{machine-id}-{layout-direction}-{machine-port}.sh`

- `layout-name`
  a symbolic name for the layout (e.g. `workdesk` - the layout for the desk I use at work)
- `machine-id`
  an id for the specific machine (e.g. `work`, `personal`, etc.)
- `layout-dir`
  either `x` (for horizontal) or `y` (for vertical)
- `machine-port`
  the port id where the display is connected (e.g. `hdmi`, `dp1`, etc.)

## How to create a new layout

I use `arandr` to visually create a layout and save it to a `*.sh` script.


## How to set it up in the system

There's a rofi extension that is set up to use a script that loads profiles from
the `~/.screenlayout` directory which is symlinked to the same dir in the
dotfiles directory (`$DOT`).

Trust me, it works!
