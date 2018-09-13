#
#  Created by Boyd Multerer on June 4, 2018.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#

defmodule Scenic.Driver.Glfw.Font do
  alias Scenic.Driver.Glfw
  alias Scenic.Cache
  require Logger

  # import IEx

  @system_fonts %{
    "roboto" => "/fonts/Roboto/Roboto-Regular.ttf",
    "roboto_mono" => "/fonts/Roboto_Mono/RobotoMono-Regular.ttf",
    "roboto_slab" => "/fonts/Roboto_Slab/RobotoSlab-Regular.ttf"
  }

  @cmd_load_font_file 0x37
  @cmd_load_font_blob 0x38
  @cmd_free_font 0x39

  # --------------------------------------------------------
  def load_font(font_name_or_key, port)

  def load_font(:roboto, port), do: load_system_font("roboto", port)
  def load_font(:roboto_mono, port), do: load_system_font("roboto_mono", port)
  def load_font(:roboto_slab, port), do: load_system_font("roboto_slab", port)
  def load_font("roboto", port), do: load_system_font("roboto", port)
  def load_font("roboto_mono", port), do: load_system_font("roboto_mono", port)
  def load_font("roboto_slab", port), do: load_system_font("roboto_slab", port)

  def load_font(cache_key, port) when is_bitstring(cache_key) do
    load_cache_font(cache_key, port)
  end

  # --------------------------------------------------------
  defp load_system_font(name, port) do
    path =
      :code.priv_dir(:scenic_driver_glfw)
      |> Path.join(@system_fonts[name])

    # should probably confirm a hash or something here
    # send the message to the C driver to load the font
    <<
      @cmd_load_font_file::unsigned-integer-size(32)-native,
      byte_size(name) + 1::unsigned-integer-size(32)-native,
      byte_size(path) + 1::unsigned-integer-size(32)-native,
      name::binary,
      # null terminate so it can be used directly
      0::size(8),
      path::binary,
      # null terminate so it can be used directly
      0::size(8)
    >>
    |> Glfw.Port.send(port)
  end

  # --------------------------------------------------------
  defp load_cache_font(font_key, port) do
    with {:ok, font_blob} <- Cache.fetch(font_key) do
      # send the message to the C driver to load the font
      <<
        @cmd_load_font_blob::unsigned-integer-size(32)-native,
        byte_size(font_key) + 1::unsigned-integer-size(32)-native,
        byte_size(font_blob)::unsigned-integer-size(32)-native,
        font_key::binary,
        # null terminate so it can be used directly
        0::size(8),
        font_blob::binary
      >>
      |> Glfw.Port.send(port)
    else
      _ ->
        Logger.error(
          "Failed to load a font from the cache.\r\n" <> "Is the key #{font_key} loaded?"
        )
    end
  end

  # --------------------------------------------------------
  def free_font(font_name_or_key, port) do
    name = to_string(font_name_or_key)
    # send the message to the C driver to load the font
    <<
      @cmd_free_font::unsigned-integer-size(32)-native,
      byte_size(name) + 1::unsigned-integer-size(16)-native,
      name::binary,
      # null terminate so it can be used directly
      0::size(8)
    >>
    |> Glfw.Port.send(port)
  end
end
