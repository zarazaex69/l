<div align="center">

![License](https://img.shields.io/badge/license-BSD--3--Clause-0D1117?style=flat-square&logo=open-source-initiative&logoColor=green&labelColor=0D1117)
![C](https://img.shields.io/badge/-C-0D1117?style=flat-square&logo=open-source-initiative&logoColor=ACB4B8&labelColor=0D1117)

</div>

## about

ldoc is an inline annotation language for sway config files. written directly in configs as comments. `l keybind` reads them and displays in a gui.

## syntax

### `# [L] text` - section header

groups binds below it. everything after `# [L]` until the next `# [L]` belongs to one section.

```
# [L] app launchers
bindsym $mod+Return exec $term # [l] launch {$term} on {$mod+Return}
bindsym $mod+Shift+b exec $web-browser # [l] open {$web-browser} on {$mod+Shift+b}

# [L] audio
bindsym --locked XF86AudioMute exec pactl ... # [l] mute/unmute on {XF86AudioMute}
```

### `# [l] text` - bind description

written inline after a command or on a standalone line. describes what the bind does in human language.

```
bindsym $mod+q kill # [l] kill window on {$mod+q}
```

### `{expression}` - variable substitution

inside `{}` variables `$name` are resolved from `~/.config/sway/conf.d/variables`. technical key names are replaced with human-readable ones.

```
# in variables:
# set $mod Mod4
# set $web-browser firefox

bindsym $mod+Shift+b exec $web-browser # [l] open {$web-browser} on {$mod+Shift+b}
```

displayed as: `open firefox on Win+Shift+b`

### key name replacements

| raw | display |
|---|---|
| `Mod4` | `Win` |
| `Mod1` | `Alt` |
| `Return` | `Enter` |
| `Escape` | `Esc` |
| `space` | `Space` |
| `minus` | `-` |
| `Print` | `PrtSc` |
| `Left` | `<-` |
| `Right` | `->` |
| `Up` | `^` |
| `Down` | `v` |
| `XF86AudioMute` | `AudioMute` |
| `XF86AudioLowerVolume` | `Vol-` |
| `XF86AudioRaiseVolume` | `Vol+` |
| `XF86AudioMicMute` | `MicMute` |
| `XF86MonBrightnessDown` | `Bright-` |
| `XF86MonBrightnessUp` | `Bright+` |

variables from `variables` file are also resolved: if `set $left h`, then `{$left}` displays `h`.

## where to write

any file in `~/.config/sway/conf.d/` -- the program scans the entire directory. sway ignores inline comments after `#`, so ldoc annotations don't break the config.

## full file example

```
# [L] focus navigation
bindsym $mod+$left focus left # [l] focus left on {$mod+$left}
bindsym $mod+$down focus down # [l] focus down on {$mod+$down}
bindsym $mod+$up focus up # [l] focus up on {$mod+$up}
bindsym $mod+$right focus right # [l] focus right on {$mod+$right}

# [L] layouts
bindsym $mod+f fullscreen # [l] fullscreen on {$mod+f}
bindsym $mod+Shift+space floating toggle # [l] toggle floating on {$mod+Shift+space}
```

<div align="center">

---

### Contact

Telegram: [zarazaex](https://t.me/zarazaexe)
<br>
Email: [zarazaex@tuta.io](mailto:zarazaex@tuta.io)
<br>
Site: [zarazaex.xyz](https://zarazaex.xyz)
<br>

</div>
