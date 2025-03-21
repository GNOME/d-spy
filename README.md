# D-Spy

A simple tool to explore [D-Bus](https://dbus.freedesktop.org/doc/dbus-specification.html) connections.

![Screenshot of d-spy](data/dspy-1.png)

## Installation

### Flathub

https://flathub.org/apps/org.gnome.dspy

```
flatpak install flathub org.gnome.dspy
flatpak run org.gnome.dspy
```

## Building from source

### Using Flatpak

```shell
flatpak --user remote-add --if-not-exists gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo
flatpak-builder --user --install-deps-from=gnome-nightly --repo=repo --install build/ org.gnome.dspy.devel.json
```

### Using GNOME Builder

```
git clone https://gitlab.gnome.org/GNOME/d-spy
flatpak run org.gnome.Builder -p ./d-spy/
```

Then click Run.
