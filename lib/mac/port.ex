#
#  Created by Boyd Multerer on 02/15/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#
# a collection of functions for handling port specific messages
#
defmodule Scenic.Driver.Mac.Port do
#  alias Scenic.Driver.Mac

  @msg_stats_id             0x01

  @cmd_close                0x20
  @cmd_query_stats          0x21
  @cmd_reshape              0x22
  @cmd_position             0x23
  @cmd_focus                0x24
  @cmd_iconify              0x25
  @cmd_maximize             0x26
  @cmd_restore              0x27
  @cmd_show                 0x28
  @cmd_hide                 0x29

  @cmd_clear_dl             0x02
  @cmd_set_root_dl          0x03
  # @cmd_new_dl_id            0x30
  # @cmd_free_dl_id           0x31
  # @cmd_new_tx_id            0x32
  # @cmd_free_tx_id           0x33
  # @cmd_put_tx_id            0x34

  @cmd_crash                0xFE

  @min_window_width         40     
  @min_window_height        20



#define   CMD_NEW_DL_ID             0x30
#define   CMD_FREE_DL_ID            0x31
#define   CMD_ROOT_DL_ID            0x32

#define   CMD_NEW_TX_ID             0x33
#define   CMD_FREE_TX_ID            0x34
#define   CMD_PUT_TX_FILE           0x35
#define   CMD_PUT_TX_RAW            0x36

  import IEx


  #============================================================================
  # send a message to the port. Not documented as it should be called by the other
  # functions, which prepare data for it.
  @doc false
  def send(msg, port) do
    try do
      Port.command(port, msg)
    rescue
      _ -> nil
    end
  end


  #============================================================================
  # all internal functions.

  # allocate a display list on the graphics card and pass it's id back up
  @doc false
  # def new_dl_id(port, timeout \\ 600) do
  #   Port.command(port, <<@cmd_new_dl_id>>)
  #   receive do
  #     {^port, {:data, <<@msg_new_dl_id :: size(8),
  #       dl_id :: unsigned-integer-size(32)-native,
  #     >> }} ->
  #       {:ok, dl_id}
  #   after 
  #     timeout -> {:err, :timeout}
  #   end
  # end

  # # free a display list on the graphics card
  # @doc false
  # def free_dl_id( port, id ) do
  #   Port.command(port, <<@cmd_free_dl_id, id :: unsigned-integer-size(32)-native>>)
  # end

  @doc false
  def clear_dl(port, dl_id) do
    Port.command(port,
      <<
        @cmd_clear_dl :: unsigned-integer-size(32)-native,
        dl_id :: unsigned-integer-size(32)-native
      >>
    )
  end

  @doc false
  def set_root_dl(port, root_dl) do
    Port.command(port,
      <<
        @cmd_set_root_dl :: unsigned-integer-size(32)-native,
        root_dl :: integer-size(32)-native
      >>
    )
  end

  #============================================================================
  # all internal functions.

  # allocate a display list on the graphics card and pass it's id back up
  # @doc false
  # def new_tx_id(port, timeout \\ 600) do
  #   Port.command(port, <<@cmd_new_tx_id>>)
  #   receive do
  #     {^port, {:data, <<@msg_new_tx_id :: size(8),
  #       tx_id :: unsigned-integer-size(32)-native,
  #     >> }} ->
  #       {:ok, tx_id}
  #   after 
  #     timeout -> {:err, :timeout}
  #   end
  # end

  # free a display list on the graphics card
  # @doc false
  # def free_tx_id( port, tx_id ) do
  #   Port.command(port, <<@cmd_free_tx_id, tx_id :: unsigned-integer-size(32)-native>>)
  # end

  # # send a texture to the renderer and associate it with the already allocated tx_id
  # @doc false
  # def put_tx_id( port, tx_id, texture ) when is_binary(texture) do
  #   Port.command(port,
  #     [
  #       <<
  #         @cmd_put_tx_id,
  #         tx_id :: unsigned-integer-size(32)-native,
  #         byte_size(texture) :: unsigned-integer-size(32)-native
  #       >>,
  #       texture
  #     ]
  #   )
  # end



  #============================================================================
  @doc false
  def handle_call( :query_stats, _from, %{port: port} = state ) do
    Port.command(port, <<@cmd_query_stats :: unsigned-integer-size(32)-native>>)
pry()
    reply = receive do
      {^port, {:data, <<@msg_stats_id :: size(8),
        input_flags :: unsigned-integer-native-size(32),
        x_pos       :: integer-native-size(32),
        y_pos       :: integer-native-size(32),
        width       :: integer-native-size(32),
        height      :: integer-native-size(32),
        focused     :: size(8),
        resizable   :: size(8),
        iconified   :: size(8),
        maximized   :: size(8),
        visible     :: size(8)
      >> }} ->
        {:ok, %{
          input_flags:  input_flags,
          x_pos:        x_pos,
          y_pos:        y_pos,
          width:        width,
          height:       height,
          focused:      focused != 0,
          resizable:    resizable != 0,
          iconified:    iconified != 0,
          maximized:    maximized != 0,
          visible:      visible != 0,
          pid:          self(),
          module:       __MODULE__
        }}
    after 
      200 -> {:err, :timeout}
    end
    {:reply, reply, state}
  end


  #============================================================================
  @doc false
  def handle_cast( msg, state)

  def handle_cast( {:reshape, {w, h}}, %{port: port} = state) when is_integer(w) and is_integer(h) do
    # enforce a minimum size...
    w = cond do
      w < @min_window_width -> @min_window_width
      true -> w
    end
    h = cond do
      h < @min_window_height -> @min_window_height
      true -> h
    end
    msg = <<
      @cmd_reshape :: unsigned-integer-size(32)-native,
      w :: integer-size(32)-native,
      h :: integer-size(32)-native
    >>
    Port.command(port, msg)
    {:noreply, state}
  end

  def handle_cast( {:position, {x, y}}, %{port: port} = state) when is_integer(x) and is_integer(y) do
    msg = <<
      @cmd_position :: unsigned-integer-size(32)-native,
      x :: integer-size(32)-native,
      y :: integer-size(32)-native
    >>
    Port.command(port, msg)
    {:noreply, state}
  end

if Mix.env() == :dev do
  def handle_cast( :crash, %{port: port} = state) do
    Port.command(port, <<@cmd_crash :: unsigned-integer-size(32)-native>>)
    {:noreply, state}
  end
end

  def handle_cast( :close, %{port: port} = state) do
    Port.command(port, <<@cmd_close :: unsigned-integer-size(32)-native>>)
    {:noreply, state}
  end

  def handle_cast( :focus, %{port: port} = state) do
    Port.command(port, <<@cmd_focus :: unsigned-integer-size(32)-native>>)
    {:noreply, state}
  end

  def handle_cast( :iconify, %{port: port} = state) do
    Port.command(port, <<@cmd_iconify :: unsigned-integer-size(32)-native>>)
    {:noreply, state}
  end

  def handle_cast( :maximize, %{port: port} = state) do
    Port.command(port, <<@cmd_maximize :: unsigned-integer-size(32)-native>>)
    {:noreply, state}
  end

  def handle_cast( :restore, %{port: port} = state) do
    Port.command(port, <<@cmd_restore :: unsigned-integer-size(32)-native>>)
    {:noreply, state}
  end

  def handle_cast( :show, %{port: port} = state) do
    Port.command(port, <<@cmd_show :: unsigned-integer-size(32)-native>>)
    {:noreply, state}
  end

  def handle_cast( :hide, %{port: port} = state) do
    Port.command(port, <<@cmd_hide :: unsigned-integer-size(32)-native>>)
    {:noreply, state}
  end

  # unhandled. do nothing
  def handle_cast( msg, _), do: msg


  #============================================================================



end