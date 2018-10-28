# Scenic.Driver.Glfw

The main Mac and Ubuntu driver for Scenic applications

Might work on other systems too, but I've only tested those two...

See main Scenic documentation for the real installation steps

### Installing on MacOS

The easiest way to install on MacOS is to use Homebrew. Just run the following in a terminal:

```bash
brew update
brew install glfw3 glew pkg-config
```


Once these components have been installed, you should be able to build the `scenic_driver_glfw` driver.

### Installing on Ubuntu

The easiest way to install on Ubuntu is to use apt-get. Just run the following:

```bash
apt-get update
apt-get install pkgconf libglfw3 libglfw3-dev libglew2.0 libglew-dev
```

Once these components have been installed, you should be able to build the `scenic_driver_glfw` driver.

### Installing on Archlinux

The easiest way to install on Archlinux is to use pacman. Just run the following:


```bash
pacman -Syu
sudo pacman -S glfw-x11 glew
```

If you're using wayland, you'll probably need `glfw-wayland` instead of `glfw-x11` and `glew-wayland` instead of `glew`

## General Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed
by adding `scenic_driver_glfw` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:scenic_driver_glfw, "~> 0.9"}
  ]
end
```

Documentation can be generated with [ExDoc](https://github.com/elixir-lang/ex_doc)
and published on [HexDocs](https://hexdocs.pm). Once published, the docs can
be found at [https://hexdocs.pm/scenic_driver_mac](https://hexdocs.pm/scenic_driver_mac).

