#
#  Created by Boyd Multerer on 02/16/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#
# a collection of functions for maintaining and drawing graphs. These could have
# just as well been in Scenic.Driver.Glfw itself, but that was getting too long
# and complicated
#
defmodule Scenic.Driver.Glfw.Cache do
  alias Scenic.Driver.Glfw
  alias Scenic.Cache.Static
  alias Scenic.Cache.Dynamic

  # @msg_new_tx_id            0x31

  @cmd_free_tx_id 0x33
  @cmd_put_tx_file 0x34
  @cmd_put_tx_raw 0x35

  # import IEx

  # ============================================================================

  # --------------------------------------------------------
  def handle_cast({Static.Texture, :put, key}, %{port: port, ready: true} = state) do
    load_static_texture(key, port)
    {:noreply, state}
  end

  # --------------------------------------------------------
  def handle_cast({Static.Texture, :delete, key}, %{port: port, ready: true} = state) do
    <<
      @cmd_free_tx_id::unsigned-integer-size(32)-native,
      byte_size(key) + 1::unsigned-integer-size(32)-native,
      key::binary,
      0::size(8)
    >>
    |> Glfw.Port.send(port)

    {:noreply, state}
  end

  # --------------------------------------------------------
  def handle_cast({Scenic.Cache.Dynamic.Texture, :put, key}, %{port: port, ready: true} = state) do
    load_dynamic_texture(key, port)
    {:noreply, state}
  end

  # --------------------------------------------------------
  def handle_cast({Scenic.Cache.Dynamic.Texture, :delete, key}, %{port: port, ready: true} = state) do
    <<
      @cmd_free_tx_id::unsigned-integer-size(32)-native,
      byte_size(key) + 1::unsigned-integer-size(32)-native,
      key::binary,
      0::size(8)
    >>
    |> Glfw.Port.send(port)

    {:noreply, state}
  end

  # --------------------------------------------------------
  # unhandled. do nothing
  def handle_cast(msg, _state) do
    msg
  end

  # ============================================================================

  # --------------------------------------------------------
  def load_static_texture(key, port) do
    # Static.Texture.subscribe(key, :all)
    with {:ok, data} <- Static.Texture.fetch(key) do
      <<
        @cmd_put_tx_file::unsigned-integer-size(32)-native,
        byte_size(key) + 1::unsigned-integer-size(32)-native,
        byte_size(data)::unsigned-integer-size(32)-native,
        key::binary,
        0::size(8),
        data::binary
      >>
      |> Glfw.Port.send(port)
    else
      err -> IO.inspect(err, label: "load_static_texture")
    end
  end

  # --------------------------------------------------------
  def load_dynamic_texture(key, port) do
    with {:ok, {type, width, height, pixels}} <- Dynamic.Texture.fetch(key) do
      depth = case type do
        :g -> 1
        :ga -> 2
        :rgb -> 3
        :rgba -> 4
      end

      <<
        @cmd_put_tx_raw::unsigned-integer-size(32)-native,
        byte_size(key) + 1::unsigned-integer-size(32)-native,
        byte_size(pixels)::unsigned-integer-size(32)-native,
        depth::unsigned-integer-size(32)-native,
        width::unsigned-integer-size(32)-native,
        height::unsigned-integer-size(32)-native,
        key::binary,
        0::size(8),
        pixels::binary
      >>
      |> Glfw.Port.send(port)
    else
      err -> IO.inspect(err, label: "load_dynamic_texture")
    end
  end

end
