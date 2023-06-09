# Scenic.Driver.Glfw

The main Mac and Ubuntu driver for Scenic applications

Might work on other systems too, but I've only tested those two...

See main Scenic documentation for the real installation steps

## ⚠ This has been replaced in Scenic 0.11+ by [`scenic_driver_local`](https://github.com/ScenicFramework/scenic_driver_local)! ⚠

### Installing on MacOS

The easiest way to install on MacOS is to use Homebrew. Just run the following in a terminal:

```bash
brew update
brew install glfw3 glew pkg-config
```


Once these components have been installed, you should be able to build the `scenic_driver_glfw` driver.

### Installing on Ubuntu 18.04

The easiest way to install on Ubuntu is to use apt-get. Just run the following:

```bash
apt-get update
apt-get install pkgconf libglfw3 libglfw3-dev libglew2.0 libglew-dev
```

Once these components have been installed, you should be able to build the `scenic_driver_glfw` driver.

### Installing on Ubuntu 20.04

The easiest way to install on Ubuntu is to use apt-get. Just run the following:

```bash
apt-get update
apt-get install pkgconf libglfw3 libglfw3-dev libglew2.1 libglew-dev
```

Once these components have been installed, you should be able to build the `scenic_driver_glfw` driver.

### Installing on Archlinux

The easiest way to install on Archlinux is to use pacman. Just run the following:


```bash
pacman -Syu
sudo pacman -S glfw-x11 glew
```

If you're using wayland, you'll probably need `glfw-wayland` instead of `glfw-x11` and `glew-wayland` instead of `glew`

### Installing on Windows

First, make sure to have installed Visual Studio with its "Desktop development with C++" package.

Next, you need to download the Windows binaries for [GLFW](https://www.glfw.org/download.html) and [GLEW](http://glew.sourceforge.net/index.html) manually.

Locate your Visual Studio installation directory. Two folders will be required for the next steps:

* The **Include** folder: `{Visual Studio Path}\VC\Tools\MSVC\{version number}\include`
* The **Lib** folder: `{Visual Studio Path}\VC\Tools\MSVC\{version number}\lib\x64`

Open the GLFW package you downloaded. Extract the contents of the packaged `include` folder to your Visual Studio **Include** folder. Next to the `include` folder, you'll also find several `lib-vc20xx` folders. Select the closest match to your Visual Studio version and extract the contents to your **Lib** folder.

Lastly, install the GLEW package. Find the packaged `include` folder and extract its contents to your **Include** folder as well. You should now have two new folders in your **Include** folder: `GL` and `GLFW`. Now navigate to `lib\Release\x64` in the GLEW package. Copy all `*.lib` files to your **Lib** folder. Finally, navigate to `bin\Release\x64` and copy `glew32.dll` to your `Windows\system32` folder.

Once these components have been installed, you should be able to build the `scenic_driver_glfw` driver.

## General Installation

As this is a hex package, it can be installed
by adding `scenic_driver_glfw` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:scenic_driver_glfw, "~> 0.10"}
  ]
end
```

## Documentation

Documentation can be found at [https://hexdocs.pm/scenic_driver_glfw](https://hexdocs.pm/scenic_driver_glfw).

