# qpwgraph User Manual

**qpwgraph** is a graph manager dedicated to [PipeWire](https://pipewire.org),
using the [Qt C++ framework](https://qt.io), based and pretty much like the
same of [QjackCtl](https://qjackctl.sourceforge.io). The source code is
available [on freedesktop.org's
GitLab](https://gitlab.freedesktop.org/rncbc/qpwgraph) and also mirrored [on
GitHub](https://github.com/rncbc/qpwgraph).

The core of the interface is a canvas showing all relevant nodes from PipeWire,
with their available ports.

## Ports

Ports are directional, they can be either:

* Source ports (i.e. output). Located at the right-most edge of a node, they
  generate an audio/video/midi stream.
* Sink ports (i.e. input). Located at the left-most edge of a node, they
  consume an audio/video/midi stream.

Ports also have different types:

* Audio (default color: green)
* Video (default color: blue)
* PipeWire/JACK MIDI (default color: red)
* ALSA MIDI (default color: purple)

Ports of the same type and opposite directions can be connected.

## Keyboard and mouse shortcuts

### Navigation

| Interaction                | Action                      |
|----------------------------|-----------------------------|
| Middle click and drag      | Pan the canvas              |
| Ctrl + left click and drag | Pan the canvas              |
|        Mouse scroll        | Pan the canvas vertically   |
| Alt  + mouse scroll        | Pan the canvas horizontally |
| Ctrl + mouse scroll        | Zoom the canvas             |
| Ctrl + Plus                | Zoom in the canvas          |
| Ctrl + Minus               | Zoom out the canvas         |
| Ctrl + 1                   | Zoom to 100%                |
| Ctrl + 0                   | Zoom to fit contents        |

### Selection

| Interaction           | Action                        |
|-----------------------|-------------------------------|
| Left click            | Select a node or a port       |
| Left click and drag   | Rectangular selection         |
| Shift + click or drag | Add to the selection          |
| Ctrl + click or drag  | Invert (toggle) the selection |
| Ctrl + A              | Select all                    |
| Ctrl + Shift + A      | Select none                   |
| Ctrl + I              | Invert selection              |

### Linking

You can link ports by left click and dragging from one port to another. You can also:

| Interaction | Action                             |
|-------------|------------------------------------|
| Insert      | Link the selected nodes or ports   |
| Ctrl + C    | Link the selected nodes or ports   |
| Delete      | Unlink the selected nodes or ports |
| Ctrl + D    | Unlink the selected nodes or ports |

### Misc.

| Interaction      | Action                     |
|------------------|----------------------------|
| Ctrl + Z         | Undo                       |
| Ctrl + Shift + Z | Redo                       |
| F2               | Rename a node or a port    |
| Double-click     | Rename a node or a port    |
| Ctrl + M         | Toggle menu bar visibility |
| Ctrl + Q         | Quit                       |
| F5               | Refresh (usually not needed, as the canvas is updated automatically) |

## Configuration file

qpwgraph will remember the position of each node, as well as their custom names.

The configuration is located at `$XDG_CONFIG_HOME/rncbc.org/qpwgraph.conf`
(usually `~/.config/rncbc.org/qpwgraph.conf`). The configuration is saved when
the application closes.

## Patchbay

qpwgraph can remember the current connections and apply them again in a later
moment. This feature is called Patchbay, and it is further documented at [How
To Use The Patchbay](qpwgraph_patchbay-user_manual.md)

---
Credits: @denilsonsa (a.k.a. Denilson Sá Maia).
