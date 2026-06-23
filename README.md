<div align="center">

```
███╗   ███╗ ██████╗██╗   ██╗ ██████╗ ███████╗███╗   ██╗
████╗ ████║██╔════╝██║   ██║██╔════╝ ██╔════╝████╗  ██║
██╔████╔██║██║     ██║   ██║██║  ███╗█████╗  ██╔██╗ ██║
██║╚██╔╝██║██║     ██║   ██║██║   ██║██╔══╝  ██║╚██╗██║
██║ ╚═╝ ██║╚██████╗╚██████╔╝╚██████╔╝███████╗██║ ╚████║
╚═╝     ╚═╝ ╚═════╝ ╚═════╝  ╚═════╝ ╚══════╝╚═╝  ╚═══╝
```

**Material Color Utilities Generator**

A fast, zero-dependency C tool that generates complete [Material You](https://m3.material.io/styles/color/overview) color schemes from any image or seed color, then applies them to any template file using a simple `{{colors.role.variant.format}}` token syntax.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg?style=flat-square)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.0-green.svg?style=flat-square)](mcugen.c)
[![Language: C](https://img.shields.io/badge/language-C11-orange.svg?style=flat-square)](mcugen.c)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-lightgrey.svg?style=flat-square)](Makefile)

</div>

---

## What is mcugen?

`mcugen` is a command-line tool that implements the full **Material Color Utilities** pipeline in pure C. You give it a wallpaper, a hex color, or an ARGB value — it extracts the dominant color, runs it through the HCT (Hue, Chroma, Tone) color space and the CAM16 perceptual appearance model, and produces a complete 52-role Material You color scheme for both light and dark modes.

It then takes your template files — CSS, RON, JSON, SCSS, anything — and replaces `{{colors.primary.dark.hex}}` style tokens with the real computed values. Hooks let you trigger reloads, recompilations, or any shell command after each template is processed.

**The entire tool is a single ~145KB executable with no runtime dependencies.**

---

## Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [Build from source](#build-from-source)
  - [Install to PATH](#install-to-path)
- [Quick Start](#quick-start)
- [CLI Reference](#cli-reference)
  - [Commands](#commands)
  - [Options](#options)
  - [Usage examples](#usage-examples)
- [Configuration](#configuration)
  - [Config file location](#config-file-location)
  - [Config syntax](#config-syntax)
  - [Template blocks](#template-blocks)
  - [Hooks](#hooks)
  - [Full example config](#full-example-config)
- [Template Syntax](#template-syntax)
  - [Token structure](#token-structure)
  - [Color roles](#color-roles)
  - [Variants](#variants)
  - [Formats](#formats)
  - [Template examples](#template-examples)
- [Color Science](#color-science)
  - [HCT color space](#hct-color-space)
  - [TonalSpot scheme](#tonalspot-scheme)
  - [Image extraction](#image-extraction)
- [Use Cases](#use-cases)
- [Project Structure](#project-structure)
- [License](#license)
- [Author](#author)

---

## Features

- **Full M3 color system** — generates all 52 Material You color roles (primary, secondary, tertiary, error, surface family, fixed roles, inverse roles, outline, scrim, shadow)
- **Both light and dark** — every role is computed for both modes; templates can embed both in the same file
- **Image input** — extracts dominant color from JPEG/PNG/BMP/GIF using k-means clustering via [stb_image](https://github.com/nothings/stb) — no external image libraries required
- **Rich token system** — 19 output formats per color role × 3 variants = 57 possible expansions per role
- **Any template format** — works with CSS, SCSS, RON, JSON, TOML, Lua, XML, Swift, Kotlin, anything text-based
- **Pre/post hooks** — optional shell commands per template (reload compositor, restart app, run post-processor)
- **Multiple templates** — one config, as many output files as you want
- **Zero runtime deps** — pure C11, links only against `libc` and `libm`
- **Single executable** — ~145KB binary, easy to copy anywhere
- **MIT licensed** — no restrictions, no attribution required

---

## Installation

### Build from source

**Prerequisites:** GCC or Clang, Make, libc, libm (standard on all Linux distros)

```bash
git clone https://github.com/MeghBadonia/mcugen
cd mcugen
make
```

That's it. The `mcugen` binary appears in the project directory.

### Install to PATH

```bash
make install
# installs to ~/.local/bin/mcugen
```

Make sure `~/.local/bin` is in your `$PATH`. Add this to your shell config if needed:

```bash
export PATH="$HOME/.local/bin:$PATH"
```

To uninstall:

```bash
make uninstall
```

---

## Quick Start

**1. Scaffold the config:**

```bash
mcugen init
```

This creates:
- `~/.config/mcugen/config.toml` — your configuration
- `~/.config/mcugen/templates/example.css` — a full CSS example template

**2. Generate from a wallpaper:**

```bash
mcugen image ~/Pictures/wallpaper.jpg
```

**3. Generate from a hex color:**

```bash
mcugen color "#6750A4"
```

**4. Preview without writing files:**

```bash
mcugen show color "#6750A4" --mode dark
```

**5. Use dark mode:**

```bash
mcugen image ~/Pictures/wallpaper.png --mode dark
```

---

## CLI Reference

### Commands

| Command | Description |
|---|---|
| `mcugen init` | Create starter config and example template |
| `mcugen image <path>` | Extract dominant color from image, generate scheme |
| `mcugen color <#hex>` | Use a specific hex color as seed, generate scheme |
| `mcugen show image <path>` | Print all 52 roles to stdout (no file output) |
| `mcugen show color <#hex>` | Print all 52 roles to stdout (no file output) |

### Options

| Flag | Short | Description |
|---|---|---|
| `--mode <light\|dark>` | `-m` | Color scheme mode (default: value from config) |
| `--config <path>` | `-c` | Use a custom config file |
| `--verbose` | `-v` | Enable diagnostic output |
| `--quiet` | `-q` | Suppress all output |
| `--help` | `-h` | Show help |
| `--version` | | Print version |

### Usage examples

```bash
# From image
mcugen image ~/Pictures/wallpaper.jpg
mcugen image ~/Pictures/wallpaper.png --mode dark
mcugen image ~/Pictures/wallpaper.jpg -m dark -v

# From color
mcugen color "#6750A4"
mcugen color "#6750A4" --mode dark
mcugen color "#1DB954" -m light
mcugen color "0xFF6750A4"           # ARGB hex also works

# Preview (no files written)
mcugen show color "#6750A4"
mcugen show color "#6750A4" -m dark
mcugen show image ~/wall.jpg -m dark

# Custom config
mcugen color "#6750A4" --config ~/dotfiles/mcugen.toml

# Quiet mode (for scripts)
mcugen image ~/wall.jpg -m dark -q
```

---

## Configuration

### Config file location

```
~/.config/mcugen/config.toml
```

Run `mcugen init` to generate a starter config automatically.

### Config syntax

The config uses a simple subset of [TOML](https://toml.io/):

```toml
[config]
default_mode = "light"    # or "dark"
```

The `[config]` section sets global defaults. `default_mode` is overridden by `--mode` on the command line.

### Template blocks

Each template is defined as a `[templates.<name>]` section:

```toml
[templates.my_template]
input_path  = "~/.config/mcugen/templates/my_file.css"
output_path = "~/.config/app/theme.css"
```

| Key | Required | Description |
|---|---|---|
| `input_path` | ✅ | Path to template file (supports `~`) |
| `output_path` | ✅ | Where to write the rendered output (supports `~`, directories created automatically) |
| `pre_hook` | ❌ | Shell command to run **before** rendering this template |
| `post_hook` | ❌ | Shell command to run **after** writing this template's output |

You can define as many `[templates.*]` blocks as you like. All are processed on every run.

### Hooks

Hooks are optional shell commands. They are run via `system()`, so you have full shell access — pipes, `&&`, environment variables, etc.

```toml
[templates.hyprland]
input_path  = "~/.config/mcugen/templates/hyprland_colors.conf"
output_path = "~/.config/hypr/colors.conf"
post_hook   = "hyprctl reload"

[templates.waybar]
input_path  = "~/.config/mcugen/templates/waybar_style.css"
output_path = "~/.config/waybar/style.css"
post_hook   = "pkill -SIGUSR2 waybar"

[templates.kitty]
input_path  = "~/.config/mcugen/templates/kitty_colors.conf"
output_path = "~/.config/kitty/colors.conf"
post_hook   = "kill -SIGUSR1 $(pidof kitty)"
```

### Full example config

```toml
# ~/.config/mcugen/config.toml

[config]
default_mode = "dark"

# CSS for a web app or browser extension
[templates.css]
input_path  = "~/.config/mcugen/templates/theme.css"
output_path = "~/Projects/myapp/src/theme.css"
post_hook   = "cd ~/Projects/myapp && npm run build:css"

# Hyprland window manager colors
[templates.hyprland]
input_path  = "~/.config/mcugen/templates/hyprland_colors.conf"
output_path = "~/.config/hypr/colors.conf"
post_hook   = "hyprctl reload"

# Waybar status bar styling
[templates.waybar]
input_path  = "~/.config/mcugen/templates/waybar_style.css"
output_path = "~/.config/waybar/style.css"
post_hook   = "pkill -SIGUSR2 waybar || true"

# Cosmic desktop theme (RON format)
[templates.cosmic]
input_path  = "~/.config/mcugen/templates/cosmic_theme.ron"
output_path = "~/.config/cosmic/com.system76.CosmicTheme.Dark/theme"
post_hook   = "cosmic-settings theme apply"

# Kitty terminal
[templates.kitty]
input_path  = "~/.config/mcugen/templates/kitty_colors.conf"
output_path = "~/.config/kitty/colors.conf"
post_hook   = "kill -SIGUSR1 $(pidof kitty 2>/dev/null) 2>/dev/null || true"

# GTK theme variables
[templates.gtk]
input_path  = "~/.config/mcugen/templates/gtk_colors.css"
output_path = "~/.themes/mcugen/gtk-3.0/colors.css"
post_hook   = "gsettings set org.gnome.desktop.interface gtk-theme mcugen"

# Rofi launcher
[templates.rofi]
input_path  = "~/.config/mcugen/templates/rofi_colors.rasi"
output_path = "~/.config/rofi/colors.rasi"
```

---

## Template Syntax

### Token structure

Every token follows this pattern:

```
{{colors.<role>.<variant>.<format>}}
```

All three parts after `colors.` are flexible:

```
{{colors.primary}}                      — hex, current mode
{{colors.primary.hex}}                  — explicit format
{{colors.primary.dark}}                 — explicit variant, default format (hex)
{{colors.primary.dark.hex}}             — fully qualified
{{colors.primary_fixed_dim.light.rgb}}  — fixed role, light variant, RGB format
```

### Color roles

All 52 Material You color roles:

**Primary family**

| Role | Description |
|---|---|
| `primary` | Main brand color |
| `on_primary` | Text/icon color on primary |
| `primary_container` | Container using primary hue |
| `on_primary_container` | Content on primary container |
| `inverse_primary` | Primary color for inverse surfaces |
| `primary_fixed` | Primary fixed (same in light/dark) |
| `primary_fixed_dim` | Dimmer fixed primary |
| `on_primary_fixed` | Content on fixed primary |
| `on_primary_fixed_variant` | Variant content on fixed primary |

**Secondary family**

| Role | Description |
|---|---|
| `secondary` | Supporting accent color |
| `on_secondary` | Content on secondary |
| `secondary_container` | Container with secondary hue |
| `on_secondary_container` | Content on secondary container |
| `secondary_fixed` | Fixed secondary |
| `secondary_fixed_dim` | Dimmer fixed secondary |
| `on_secondary_fixed` | Content on fixed secondary |
| `on_secondary_fixed_variant` | Variant content on fixed secondary |

**Tertiary family**

| Role | Description |
|---|---|
| `tertiary` | Complementary accent (hue + 60°) |
| `on_tertiary` | Content on tertiary |
| `tertiary_container` | Container with tertiary hue |
| `on_tertiary_container` | Content on tertiary container |
| `tertiary_fixed` | Fixed tertiary |
| `tertiary_fixed_dim` | Dimmer fixed tertiary |
| `on_tertiary_fixed` | Content on fixed tertiary |
| `on_tertiary_fixed_variant` | Variant content on fixed tertiary |

**Error family**

| Role | Description |
|---|---|
| `error` | Error state color |
| `on_error` | Content on error |
| `error_container` | Error container |
| `on_error_container` | Content on error container |

**Surface family**

| Role | Description |
|---|---|
| `surface` | Default surface color |
| `surface_dim` | Dimmer surface variant |
| `surface_bright` | Brighter surface variant |
| `surface_container_lowest` | Lowest emphasis container |
| `surface_container_low` | Low emphasis container |
| `surface_container` | Default container |
| `surface_container_high` | High emphasis container |
| `surface_container_highest` | Highest emphasis container |
| `on_surface` | Content on surface |
| `surface_variant` | Variant surface (neutral-variant palette) |
| `on_surface_variant` | Content on surface variant |
| `surface_tint` | Tint overlay color |

**Background**

| Role | Description |
|---|---|
| `background` | Page / screen background |
| `on_background` | Content on background |

**Outline**

| Role | Description |
|---|---|
| `outline` | Border / separator |
| `outline_variant` | Subtle border |

**Inverse / Misc**

| Role | Description |
|---|---|
| `inverse_surface` | Inverted surface (for snackbars, tooltips) |
| `inverse_on_surface` | Content on inverse surface |
| `scrim` | Modal overlay scrim |
| `shadow` | Drop shadow color |

### Variants

| Variant | Description |
|---|---|
| `default` | Uses the current `--mode` (light or dark) |
| `light` | Always the light-mode value |
| `dark` | Always the dark-mode value |

Useful for templates that embed both modes at once (e.g. a CSS file with `:root` and `.dark` blocks):

```css
:root {
  --primary: {{colors.primary.light.hex}};
}
.dark {
  --primary: {{colors.primary.dark.hex}};
}
```

### Formats

| Format | Example output | Description |
|---|---|---|
| `hex` | `#6750a4` | Lowercase hex with `#` prefix |
| `hex_strip` / `hex_stripped` | `6750a4` | Lowercase hex, no `#` |
| `HEX` / `hex_upper` | `#6750A4` | Uppercase hex with `#` prefix |
| `HEX_STRIP` / `hex_strip_upper` | `6750A4` | Uppercase hex, no `#` |
| `rgb` | `rgb(103, 80, 164)` | CSS `rgb()` function |
| `rgba` | `rgba(103, 80, 164, 1.00)` | CSS `rgba()` function |
| `rgb_raw` | `103, 80, 164` | Bare R, G, B values |
| `r` | `103` | Red channel (0–255) |
| `g` | `80` | Green channel (0–255) |
| `b` | `164` | Blue channel (0–255) |
| `r_float` | `0.403922` | Red channel (0.0–1.0) |
| `g_float` | `0.313725` | Green channel (0.0–1.0) |
| `b_float` | `0.643137` | Blue channel (0.0–1.0) |
| `argb_int` | `-10003548` | Signed 32-bit ARGB decimal |
| `argb_hex` | `0xFF6750A4` | Hex ARGB with `0x` prefix |
| `hsl` | `hsl(264.0, 34.4%, 47.1%)` | CSS `hsl()` function |
| `hct_hue` | `299.0489` | HCT hue (0.0–360.0) |
| `hct_chroma` | `47.9023` | HCT chroma (0.0–~130.0) |
| `hct_tone` | `40.1234` | HCT tone / L* (0.0–100.0) |

### Template examples

**CSS custom properties:**

```css
/* {{colors.primary.hex}} */
:root {
  --md-primary:          {{colors.primary.hex}};
  --md-on-primary:       {{colors.on_primary.hex}};
  --md-primary-rgb:      {{colors.primary.rgb_raw}};
  --md-surface:          {{colors.surface.hex}};
  --md-background:       {{colors.background.hex}};
  --md-outline:          {{colors.outline.hex}};
}

@media (prefers-color-scheme: dark) {
  :root {
    --md-primary:        {{colors.primary.dark.hex}};
    --md-on-primary:     {{colors.on_primary.dark.hex}};
    --md-surface:        {{colors.surface.dark.hex}};
    --md-background:     {{colors.background.dark.hex}};
  }
}
```

**Hyprland `colors.conf`:**

```ini
$primary        = rgb({{colors.primary.hex_strip}})
$on_primary     = rgb({{colors.on_primary.hex_strip}})
$background     = rgb({{colors.background.hex_strip}})
$surface        = rgb({{colors.surface.hex_strip}})
$outline        = rgb({{colors.outline.hex_strip}})
```

**Waybar `style.css`:**

```css
* {
  color:       {{colors.on_surface.hex}};
  background:  {{colors.surface_container.hex}};
}
#workspaces button.active {
  background:  {{colors.primary_container.hex}};
  color:       {{colors.on_primary_container.hex}};
}
```

**Kitty `colors.conf`:**

```conf
background        {{colors.surface.hex}}
foreground        {{colors.on_surface.hex}}
selection_background {{colors.primary_container.hex}}
selection_foreground {{colors.on_primary_container.hex}}
color0            {{colors.surface_dim.hex}}
color1            {{colors.error.hex}}
color2            {{colors.tertiary.hex}}
color3            {{colors.secondary.hex}}
color4            {{colors.primary.hex}}
```

**Cosmic `.ron` file:**

```ron
(
  name: "mcugen",
  background: (
    default: Srgb(({{colors.background.r_float}}, {{colors.background.g_float}}, {{colors.background.b_float}})),
  ),
  primary: (
    default: Srgb(({{colors.primary.r_float}}, {{colors.primary.g_float}}, {{colors.primary.b_float}})),
    on: Srgb(({{colors.on_primary.r_float}}, {{colors.on_primary.g_float}}, {{colors.on_primary.b_float}})),
  ),
)
```

**SCSS variables:**

```scss
$primary:              {{colors.primary.hex}};
$primary-rgb:          {{colors.primary.rgb_raw}};
$primary-container:    {{colors.primary_container.hex}};
$on-primary:           {{colors.on_primary.hex}};
$secondary:            {{colors.secondary.hex}};
$surface:              {{colors.surface.hex}};
$surface-hsl:          {{colors.surface.hsl}};
$error:                {{colors.error.hex}};
```

**JSON (for apps, Electron, etc.):**

```json
{
  "colors": {
    "primary":         "{{colors.primary.hex}}",
    "onPrimary":       "{{colors.on_primary.hex}}",
    "primaryArgb":     {{colors.primary.argb_int}},
    "surface":         "{{colors.surface.hex}}",
    "surfaceRgb":      [{{colors.surface.r}}, {{colors.surface.g}}, {{colors.surface.b}}],
    "hctHue":          {{colors.primary.hct_hue}},
    "hctChroma":       {{colors.primary.hct_chroma}}
  }
}
```

---

## Color Science

### HCT color space

mcugen implements the **HCT color space** — the perceptual color model behind Material You. HCT stands for:

- **H** — Hue from CAM16, the color appearance model developed from decades of human perception research
- **C** — Chroma from CAM16, the colorfulness relative to a white reference
- **T** — Tone, equivalent to L* from CIELAB, a perceptually uniform measure of lightness

Unlike RGB or HSL, HCT is designed so that colors with the same tone have the same perceived contrast against white or black — which is why M3 can guarantee WCAG accessibility ratios by simply specifying tone deltas.

### TonalSpot scheme

mcugen uses the **TonalSpot variant** — the default Material You scheme:

| Palette | Hue | Chroma |
|---|---|---|
| Primary | seed hue | 36 |
| Secondary | seed hue | 16 |
| Tertiary | seed hue + 60° | 24 |
| Neutral | seed hue | 6 |
| Neutral Variant | seed hue | 8 |
| Error | 25° (fixed red) | 84 |

Each palette is a tonal spectrum — the same hue and chroma at every tone from 0 (black) to 100 (white). Color roles are then assigned specific tones from these palettes based on whether you're in light or dark mode.

### Image extraction

When given an image, mcugen:

1. Decodes the image (JPEG, PNG, BMP, or GIF) using `stb_image.h`
2. Subsamples to at most 4096 pixels using uniform stride sampling
3. Runs **k-means clustering** with k=8 centroids for up to 20 iterations
4. Scores each centroid by `count × chroma × brightness_weight` where `brightness_weight` peaks at luminance ≈ 0.45 — favoring vivid mid-tone colors over dark or washed-out ones
5. Uses the winning centroid's RGB as the seed color

This produces the same class of result as Android's Monet wallpaper color extraction — a chromatic, representative color that drives the entire palette.

---

## Use Cases

**Desktop theming on Hyprland / Sway / i3**

Add to your wallpaper-change script:

```bash
swww img ~/wall.jpg && mcugen image ~/wall.jpg --mode dark
```

**Dotfiles automation**

Keep all your app configs as templates and regenerate on demand:

```bash
mcugen color "#$(xcolor)" --mode dark    # pick from screen, apply everywhere
```

**Consistent multi-app theming**

One config file drives Waybar, Kitty, Rofi, GTK, Firefox, and any other app simultaneously.

**Web development**

Generate a full CSS custom-property file from your brand color, both light and dark, in milliseconds:

```bash
mcugen color "#0057B7" --mode light > /dev/null
# your CSS file is already updated
```

**CI/CD design tokens**

Integrate into a build pipeline to generate design tokens from a brand color:

```bash
mcugen color "#$(cat brand_color.txt)" -q -m light
mcugen color "#$(cat brand_color.txt)" -q -m dark
```

---

## Project Structure

```
mcugen/
├── mcugen.c                   Single-file main executable
├── mcu.h                      Umbrella header for MCU library
├── stb_image.h                Image decoding (Sean Barrett, MIT/PD)
├── Makefile
│
├── utils/
│   ├── math_utils.c/h         lerp, sanitizeDegrees, matrixMultiply
│   ├── color_utils.c/h        sRGB↔XYZ↔Lab↔L*, linearize/delinearize
│   └── string_utils.c/h       Hex string formatting
│
├── hct/
│   ├── viewing_conditions.c/h CAM16 environment parameters
│   ├── cam16.c/h              CAM16 color appearance model
│   ├── hct_solver.c/h         Newton + bisection gamut solver
│   └── hct.c/h                HCT struct: from(), setHue/Chroma/Tone
│
├── blend/
│   └── blend.c/h              harmonize(), hctHue(), cam16Ucs()
│
├── contrast/
│   └── contrast.c/h           WCAG contrast ratio utilities
│
├── dislike/
│   └── dislike.c/h            Dark yellow-green avoidance filter
│
├── palettes/
│   ├── tonal_palette.c/h      TonalPalette with 101-slot cache
│   └── core_palettes.h        CorePalettes (5 TonalPalette) struct
│
├── score/
│   └── score.c/h              Color ranking for image quantization
│
└── temperature/
    └── temperature_cache.c/h  Analogous + complementary colors
```

---

## License

This project is released under the **MIT License**.

The algorithms in this codebase implement published color-science specifications: CAM16 (Li et al., 2017), CIELAB, WCAG 2.x contrast ratio, and the HCT color space. Mathematical formulas and scientific constants are not copyrightable.

The C implementation was automatically ported from the [official Kotlin sources](https://github.com/material-foundation/material-color-utilities) by Claude (Anthropic) and substantially modified.

`stb_image.h` is by Sean Barrett and contributors — MIT License / Public Domain. Its own license notice is preserved inside the file.

---

## Author

**Megh Badonia**

- 📧 [badoniamegh@gmail.com](mailto:badoniamegh@gmail.com)
- 🐙 [github.com/MeghBadonia](https://github.com/MeghBadonia)

---

<div align="center">

Made with precision and a lot of HCT math.

*If it themes your desktop, it's doing its job.*

</div>
