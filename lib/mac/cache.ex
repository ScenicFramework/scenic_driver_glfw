#
#  Created by Boyd Multerer on 02/16/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#
# a collection of functions for maintaining and drawing graphs. These could have
# just as well been in Scenic.Driver.Mac itself, but that was getting too long
# and complicated
#
defmodule Scenic.Driver.Mac.Cache do
  alias Scenic.Driver.Mac
  alias Scenic.Cache

  # @msg_new_tx_id            0x31

  @cmd_free_tx_id           0x33
  @cmd_put_tx_file          0x34
  @cmd_put_tx_raw           0x35

#  import IEx

  #============================================================================

  #--------------------------------------------------------
  def handle_cast( {:cache_put, key}, %{port: port, ready: true} = state ) do
    load_texture( key, port )
    {:noreply, state}
  end

  #--------------------------------------------------------
  def handle_cast( {:cache_delete, key}, %{port: port, ready: true} = state) do
    <<
      @cmd_free_tx_id,
      byte_size(key) + 1 :: unsigned-integer-size(32)-native,
      key :: binary,
      0 :: size(8)
    >>
    |> Mac.Port.send( port )
    {:noreply, state}
  end

  #--------------------------------------------------------
  # unhandled. do nothing
  def handle_cast( msg, _state ) do
    msg
  end

  #============================================================================
  
  #--------------------------------------------------------
  def load_texture( key, port ) do
    with {:ok, data} <- Cache.fetch(key) do
      data = case data do
        {:texture, data}-> data
        data -> data
      end
      send_texture( key, data, port )
    end
  end

  #--------------------------------------------------------
  defp send_texture( _key, {width, height, depth, pixels}, port ) do
    <<
      @cmd_put_tx_raw,
      width :: unsigned-integer-size(32)-native,
      height :: unsigned-integer-size(32)-native,
      depth :: unsigned-integer-size(8),
      byte_size(pixels) :: unsigned-integer-size(32)-native,
      pixels :: binary
    >>
    |> Mac.Port.send( port )
  end

  #--------------------------------------------------------
  defp send_texture( key, data, port ) when is_binary(data) do
    <<
      @cmd_put_tx_file,
      byte_size(key) + 1 :: unsigned-integer-size(32)-native,
      byte_size(data) :: unsigned-integer-size(32)-native,
      key :: binary,
      0 :: size(8),
      data :: binary
    >>
    |> Mac.Port.send( port )
  end

end





















