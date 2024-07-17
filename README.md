[![Build Status](https://github.com/tsoding/sowon/workflows/CI/badge.svg)](https://github.com/tsoding/sowon/actions)

# Sowon

![demo](./demo.gif)

## Build

Dependencies: [SDL2](https://www.libsdl.org/download-2.0.php)

### Debian
```console
$ sudo apt-get install libsdl2-dev
$ make
```

### MacOS

```console
$ brew install sdl2 pkg-config
$ make
```

### Windows

#### Visual Studio

- Enter the Visual Studio Command Line Development Environment https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line
  - Basically just find `vcvarsall.bat` and run `vcvarsall.bat x64` inside of cmd
- Download [SDL2 VC Development Libraries](https://libsdl.org/release/SDL2-devel-2.0.12-VC.zip) and copy it to `path\to\sowon`

```console
> cd path\to\sowon
> tar -xf SDL2-devel-2.0.12-VC.zip
> move SDL2-2.0.12 SDL2
> del SDL2-devel-2.0.12-VC.zip
> build_msvc
```

## Usage

### Modes

- Ascending mode: &emsp; `./sowon`
- Descending mode: &emsp;`./sowon <seconds>`,  
  &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;`./sowon <seconds>s`,  
  &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;`./sowon <minutes>m`,  
  &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;`./sowon <hours>h`,  
  &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;`./sowon <hours>h<minutes>m<seconds>s`

- Clock Mode: &emsp;&emsp;&emsp;&emsp; `./sowon clock`


### Flags

- Start in paused state: &emsp;&emsp;&emsp;`./sowon -p <mode>`
- Exit after coundown ends: &emsp;`./sowon -e <seconds>`

### Key bindings

| Key | Description |
| --- | --- |
| <kbd>SPACE</kbd> | Toggle pause |
| <kbd>=</kbd> | Zoom in |
| <kbd>-</kbd> | Zoom out |
| <kbd>0</kbd> | Zoom 100% |
| <kbd>F5</kbd> | Restart |
| <kbd>F11</kbd> | Fullscreen |
