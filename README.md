# OBS Projector Plugin

## Introduction

The plugin is meant to be used for blending projectors taking OBS Studio program output as source

## Supported Build Environments

| Platform  | Tool   | Status    |
|-----------|--------| --------- |
| Windows   | Visal Studio 17 2022 | Not Tested |
| macOS     | XCode 16.0 | Not Tested |
| Windows, macOS  | CMake 3.30.5 | Not Tested |
| Ubuntu 24.04 | CMake 3.28.3 | OK |
| Ubuntu 24.04 | `ninja-build` | Not Tested |
| Ubuntu 24.04 | `pkg-config` | Not Tested |
| Ubuntu 24.04 | `build-essential` | Not Tested |

## Quick Start

### Ubuntu 24.04 - CMake 3.28.3

- Run `cmake --preset ubuntu-x86_64`
- Run `cmake --build --preset ubuntu-x86_64`
- Install the plugin `.so` module.

## Documentation

All documentation can be found in the [OBS Projector Plugin Wiki](https://github.com/julia-otran/obs-projector/wiki).
