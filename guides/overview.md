Scenic.Driver.Glfw is the main driver for windowing operating systems.

It performs both rendering of the graphs to the screen and collects user input
to send up to the ViewPort.

So far it has been built and run on quite a few including the Macintosh,
Ubuntu, ArchLinux, Fedora and more are being investigated.

If you are curious, the "Glfw" part of the name comes from
[Graphics Library for Windows](https://www.glfw.org/), which this driver
uses to manage the windows. This is nice as it helps keeps the driver code
the same across the various Operating Systems.

## Install Dependencies

The design of Scenic goes to great lengths to minimize its dependencies to just
the minimum. Namely, it needs Erlang/Elixir and OpenGL.

Rendering your application into a window on your local computer (MacOS, Ubuntu
and others) is done by the `scenic_driver_glfw` driver. It uses the GLFW and
GLEW libraries to connect to OpenGL.

The instructions below assume you have already installed Elixir/Erlang. If you
need to install Elixir/Erlang there are instructions on the [elixir-lang
website](https://elixir-lang.org/install.html).

### Installing on MacOS

The easiest way to install on MacOS is to use Homebrew. Just run the following
in a terminal:

```bash
brew update
brew install glfw3 glew pkg-config
```

Once these components have been installed, you should be able to build the
`scenic_driver_glfw` driver.

### Installing on Ubuntu 16

The easiest way to install on Ubuntu is to use apt-get. Just run the following:

```bash
sudo apt-get update
sudo apt-get install pkgconf libglfw3 libglfw3-dev libglew1.13 libglew-dev
```

Once these components have been installed, you should be able to build the
`scenic_driver_glfw` driver.

### Installing on Ubuntu 18

The easiest way to install on Ubuntu is to use apt-get. Just run the following:

```bash
sudo apt-get update
sudo apt-get install pkgconf libglfw3 libglfw3-dev libglew2.0 libglew-dev
```

Once these components have been installed, you should be able to build the
`scenic_driver_glfw` driver.

### Installing on Fedora

The easiest way to install on Fedora is to use dnf. Just run the following:

```
dnf install glfw glfw-devel pkgconf glew glew-devel
```

Once these components have been installed, you should be able to build the `scenic_driver_glfw` driver.

### Installing on Archlinux

The easiest way to install on Archlinux is to use pacman. Just run the following:

```bash
sudo pacman -S glfw-x11 glew
```

If you're using wayland, you'll probably need `glfw-wayland` instead of `glfw-x11` and `glew-wayland` instead of `glew`

## Configuration

## Compatibility

Unlike the rest of Scenic, the drivers do make assumptions about your specific hardware.
If it crashes or fails at startup, then it may be requesting something that is not
compatible with your video card.

In this case, please file an issue in GitHub and include the exact version of
your operating system and the exact video card you are using. I cannot guarantee that
we can figure out every video card, but it is worth at least a try.
