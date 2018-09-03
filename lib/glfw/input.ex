#
#  Created by Boyd Multerer on 02/15/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#
# a collection of functions for handling messages from the port
#
defmodule Scenic.Driver.Glfw.Input do
  use Bitwise

  alias Scenic.Driver.Glfw.Cache
  alias Scenic.Driver.Glfw.Font
  alias Scenic.ViewPort
  alias Scenic.Utilities

  require Logger

  # import IEx

  # incoming message ids
  @msg_close_id             0x00
#  @msg_stats_id             0x01
  @msg_puts_id              0x02
  @msg_write_id             0x03
  @msg_inspect_id           0x04
  @msg_reshape_id           0x05
  @msg_ready_id             0x06
  @msg_draw_ready_id        0x07

  @msg_key_id               0x0A
  @msg_char_id              0x0B
  @msg_cursor_pos_id        0x0C
  @msg_mouse_button_id      0x0D
  @msg_mouse_scroll_id      0x0E
  @msg_cursor_enter_id      0x0F
#  @msg_drop_paths_id        0x10
  @msg_cache_miss           0x20

  @msg_font_miss            0x22


  @debounce_speed           16

  #============================================================================
  # debounce input

    #--------------------------------------------------------
  def handle_debounce( type, %{
    draw_busy: true
  } = state ) do
    # effectively skip a frame
    Process.send_after(self(), {:debounce, type}, @debounce_speed)
    {:noreply, state}
  end

  def handle_debounce( type, %{
    debounce: debounce,
    viewport:  viewport,
    draw_busy: false
  } = state ) do
    case debounce[type] do
      nil -> :ok
      msg ->
        ViewPort.input( viewport, msg )
    end
    state = Utilities.Map.delete_in(state, [:debounce, type])
    {:noreply, state}
  end

  #--------------------------------------------------------
  defp debounce_input({type, _} = msg, %{
    debounce: debounce
  } = state) do

    state = case debounce[type] do
      nil ->
        Process.send_after(self(), {:debounce, type}, @debounce_speed)
        put_in(state, [:debounce, type], msg)
      _ ->
        put_in(state, [:debounce, type], msg)
    end

    { :noreply, state }
  end


  #============================================================================

  @doc false
  def handle_port_message( msg, state )

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_ready_id :: unsigned-integer-size(32)-native,
    start_dl :: integer-size(32)-native
  >>, %{
    viewport: viewport,
    dl_block_size: dl_block_size
  } = state ) do
    state = state
    |> Map.put( :ready, true )
    |> Map.put( :start_dl, start_dl )
    |> Map.put( :end_dl, start_dl + dl_block_size - 1 )
    |> Map.put( :last_used_dl, start_dl )
    
    # |> Glfw.Font.initialize()

    GenServer.cast(viewport, {:driver_ready, self()})

    { :noreply, state }
  end

  #--------------------------------------------------------
  # this feels like it should live in graph.ex, but it is input from
  # the driver, so it is here...
  def handle_port_message( <<
    @msg_draw_ready_id :: unsigned-integer-size(32)-native,
    id :: unsigned-integer-size(32)-native
  >>, %{
    currently_drawing: currently_drawing
  } = state ) do
    # remove the id from the currently_drawing list
    currently_drawing = List.delete(currently_drawing, id)
    state = %{state | currently_drawing: currently_drawing}

    IO.puts "done drawing #{inspect(id)}"

    # if the current draw is done, mark it so
    case currently_drawing do
      [] -> {:noreply, %{state | draw_busy: false}}
      _ -> {:noreply, state}
    end
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_reshape_id :: unsigned-integer-size(32)-native,
    window_width :: unsigned-integer-size(32)-native,
    window_height :: unsigned-integer-size(32)-native,
    frame_width :: unsigned-integer-size(32)-native,
    frame_height :: unsigned-integer-size(32)-native
  >>, state ) do
    state = state
    |> Map.put( :window, {window_width, window_height} )
    |> Map.put( :frame, {frame_width, frame_height} )
    |> Map.put( :screen_factor, frame_width / window_width )

    debounce_input({:viewport_reshape, {window_width, window_height}}, state)

    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_close_id :: unsigned-integer-size(32)-native>>,
    %{viewport: viewport} = state
  ) do
    GenServer.cast(viewport, :user_close)
    { :noreply, Map.put(state, :closing, true) }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_puts_id :: unsigned-integer-size(32)-native>> <> msg,
  state ) do
    IO.puts( msg )
    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_write_id :: unsigned-integer-size(32)-native>> <> msg,
  state ) do
    IO.write( msg )
    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message(<<
    @msg_inspect_id :: unsigned-integer-size(32)-native>> <> msg,
  state ) do
    IO.inspect(msg)
    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message(
    <<
      @msg_cache_miss :: unsigned-integer-size(32)-native
    >> <> key,
    %{port: port} = state )
  do
    Cache.load_texture( key, port )
    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_font_miss :: unsigned-integer-size(32)-native>> <> key,
    %{port: port} = state )
  do
    Font.load_font( key, port )
    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_key_id :: unsigned-integer-size(32)-native,
    key       :: unsigned-integer-native-size(32),
    _scancode :: unsigned-integer-native-size(32),
    action    :: unsigned-integer-native-size(32),
    mods      :: unsigned-integer-native-size(32)
  >>, %{viewport: viewport} = state ) do
    key = key_to_name(key)
    action = action_to_atom(action)

    ViewPort.input( viewport, {:key, {key, action, mods}} )

    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_char_id :: unsigned-integer-size(32)-native,
    codepoint :: unsigned-integer-native-size(32),
    mods      :: unsigned-integer-native-size(32),
  >>, %{viewport: viewport} = state ) do
    codepoint = codepoint_to_char(codepoint)

    ViewPort.input( viewport, {:codepoint, {codepoint, mods}} )

    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_cursor_pos_id :: unsigned-integer-size(32)-native,
    x :: float-native-size(32),
    y :: float-native-size(32)
  >>, state ) do
    debounce_input({:cursor_pos, {x,y}}, state)
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_mouse_button_id :: unsigned-integer-size(32)-native,
    button  :: unsigned-integer-native-size(32),
    action  :: unsigned-integer-native-size(32),
    mods    :: unsigned-integer-native-size(32),
    x       :: float-native-size(32),
    y       :: float-native-size(32),
  >>, %{viewport: viewport} = state ) do
    button = button_to_atom(button)
    action = action_to_atom(action)

    ViewPort.input( viewport, {:cursor_button, {button, action, mods, {x,y}}} )

    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_mouse_scroll_id :: unsigned-integer-size(32)-native,
    x_offset  :: float-native-size(32),
    y_offset  :: float-native-size(32),
    x_pos     :: float-native-size(32),
    y_pos     :: float-native-size(32)
  >>, state ) do

    debounce_input({:cursor_scroll, {{x_offset, y_offset}, {x_pos,y_pos}}}, state)

    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_cursor_enter_id :: unsigned-integer-size(32)-native,
    0   :: integer-native-size(32),
    x_pos     :: float-native-size(32),
    y_pos     :: float-native-size(32)
  >>, %{viewport: viewport} = state ) do

    ViewPort.input( viewport, {:viewport_exit, {x_pos,y_pos}} )

    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    @msg_cursor_enter_id :: unsigned-integer-size(32)-native,
    1   :: integer-native-size(32),
    x_pos     :: float-native-size(32),
    y_pos     :: float-native-size(32)
  >>, %{viewport: viewport} = state ) do

    ViewPort.input( viewport, {:viewport_enter, {x_pos,y_pos}} )

    { :noreply, state }
  end

  #--------------------------------------------------------
  def handle_port_message( <<
    id :: unsigned-integer-size(32)-native,
    bin :: binary >>, state )
  do
    IO.puts "Unhandled port messages id: #{id}, msg: #{inspect(bin)}"
    { :noreply, state }
  end


  #============================================================================
  # utilities to translate Glfw input to standardized input




  #============================================================================
  # keyboard input helpers
  # these are for reading the keyboard directly. If you are trying to do text input
  # use the text/char helpers instead

  # key codes use the standards defined by Glfw
  # http://www.Glfw.org/docs/latest/group__keys.html

  #--------------------------------------------------------
  defp key_to_name( key_code )

  defp key_to_name( key ) when key < 128,   do: << key :: size(8)>>

  defp key_to_name( 161 ),   do: "world_1"              # non-US #1
  defp key_to_name( 162 ),   do: "world_2"              # non-US #2

  defp key_to_name( 256 ),   do: "escape"
  defp key_to_name( 257 ),   do: "enter"
  defp key_to_name( 258 ),   do: "tab"
  defp key_to_name( 259 ),   do: "backspace"
  defp key_to_name( 260 ),   do: "insert"
  defp key_to_name( 261 ),   do: "delete"

  defp key_to_name( 262 ),   do: "right"
  defp key_to_name( 263 ),   do: "left"
  defp key_to_name( 264 ),   do: "down"
  defp key_to_name( 265 ),   do: "up"
  defp key_to_name( 266 ),   do: "page_up"
  defp key_to_name( 267 ),   do: "page_down"
  defp key_to_name( 268 ),   do: "home"
  defp key_to_name( 269 ),   do: "end"

  defp key_to_name( 280 ),   do: "caps_lock"
  defp key_to_name( 281 ),   do: "scroll_lock"
  defp key_to_name( 282 ),   do: "num_lock"

  defp key_to_name( 283 ),   do: "print_screen"
  defp key_to_name( 284 ),   do: "pause"

  defp key_to_name( 290 ),   do: "f1"
  defp key_to_name( 291 ),   do: "f2"
  defp key_to_name( 292 ),   do: "f3"
  defp key_to_name( 293 ),   do: "f4"
  defp key_to_name( 294 ),   do: "f5"
  defp key_to_name( 295 ),   do: "f6"
  defp key_to_name( 296 ),   do: "f7"
  defp key_to_name( 297 ),   do: "f8"
  defp key_to_name( 298 ),   do: "f9"
  defp key_to_name( 299 ),   do: "f10"
  defp key_to_name( 300 ),   do: "f11"
  defp key_to_name( 301 ),   do: "f12"
  defp key_to_name( 302 ),   do: "f13"
  defp key_to_name( 303 ),   do: "f14"
  defp key_to_name( 304 ),   do: "f15"
  defp key_to_name( 305 ),   do: "f16"
  defp key_to_name( 306 ),   do: "f17"
  defp key_to_name( 307 ),   do: "f18"
  defp key_to_name( 308 ),   do: "f19"
  defp key_to_name( 309 ),   do: "f20"
  defp key_to_name( 310 ),   do: 'f21'
  defp key_to_name( 311 ),   do: "f22"
  defp key_to_name( 312 ),   do: "f23"
  defp key_to_name( 313 ),   do: "f24"
  defp key_to_name( 314 ),   do: "f25"

  defp key_to_name( 320 ),   do: "kp_0"
  defp key_to_name( 321 ),   do: "kp_1"
  defp key_to_name( 322 ),   do: "kp_2"
  defp key_to_name( 323 ),   do: "kp_3"
  defp key_to_name( 324 ),   do: "kp_4"
  defp key_to_name( 325 ),   do: "kp_5"
  defp key_to_name( 326 ),   do: "kp_6"
  defp key_to_name( 327 ),   do: "kp_7"
  defp key_to_name( 328 ),   do: "kp_8"
  defp key_to_name( 329 ),   do: "kp_9"

  defp key_to_name( 330 ),   do: "kp_decimal"
  defp key_to_name( 331 ),   do: "kp_divide"
  defp key_to_name( 332 ),   do: "kp_multiply"
  defp key_to_name( 333 ),   do: "kp_subtract"
  defp key_to_name( 334 ),   do: "kp_add"
  defp key_to_name( 335 ),   do: "kp_enter"
  defp key_to_name( 336 ),   do: "kp_equal"

  defp key_to_name( 340 ),   do: "left_shift"
  defp key_to_name( 341 ),   do: "left_control"
  defp key_to_name( 342 ),   do: "left_alt"
  defp key_to_name( 343 ),   do: "left_super"

  defp key_to_name( 344 ),   do: "right_shift"
  defp key_to_name( 345 ),   do: "right_control"
  defp key_to_name( 346 ),   do: "right_alt"
  defp key_to_name( 347 ),   do: "right_super"

  defp key_to_name( 348 ),   do: "menu"

  defp key_to_name( key ) do
    IO.puts "Driver.Glfw recieved unknown input key value: #{inspect(key)}"
    "unknown"
  end


  #--------------------------------------------------------
  # defined to follow the Glfw modifier keys
  # http://www.Glfw.org/docs/latest/group__mods.html

#  @key_mod_shift    0x0001
#  @key_mod_control  0x0002
#  @key_mod_alt      0x0004
#  @key_mod_super    0x0008
#  @key_mods         [
#    {@key_mod_shift,   :shift},
#    {@key_mod_control, :control},
#    {@key_mod_alt,     :alt},
#    {@key_mod_super,   :super}
#  ]
#
#  defp mods_to_atoms( key_mods )
#  defp mods_to_atoms( key_mods ) when is_integer(key_mods) do
#    Enum.reduce(@key_mods, [], fn({mask,mod_atom}, acc) -> 
#        case Bitwise.band(mask, key_mods) do
#          0 -> acc
#          _ -> [mod_atom | acc]
#        end
#    end)
#  end
#  defp mods_to_atoms( mods ) do
#    raise "Driver.Glfw recieved unknown mods: #{inspect(mods)}"
#  end

  #--------------------------------------------------------
  defp action_to_atom( action )
  defp action_to_atom( 0 ),  do: :release
  defp action_to_atom( 1 ),  do: :press
  defp action_to_atom( 2 ),  do: :repeat
  defp action_to_atom( _ ),  do: :unknown

  #--------------------------------------------------------
  defp codepoint_to_char( codepoint_to_atom )
  defp codepoint_to_char( cp ),  do: << cp :: utf8 >>


  #--------------------------------------------------------
  defp button_to_atom( 0 ), do: :left
  defp button_to_atom( 1 ), do: :right
  defp button_to_atom( _ ), do: :unknown

  #--------------------------------------------------------
#  defp input_type_to_flags( type )
#  defp input_type_to_flags( types ) when is_list(types) do
#    Enum.reduce(types, 0, &(input_type_to_flags(&1) ||| &2) )
#  end
#  defp input_type_to_flags( :key ),            do: 0x0001
#  defp input_type_to_flags( :codepoint ),      do: 0x0002
#  defp input_type_to_flags( :cursor_pos ),     do: 0x0004
#  defp input_type_to_flags( :cursor_button ),   do: 0x0008
#  defp input_type_to_flags( :cursor_scroll ),   do: 0x0010
#  defp input_type_to_flags( :cursor_enter ),    do: 0x0020
#  defp input_type_to_flags( :all ),            do: 0xFFFF
#  defp input_type_to_flags( type ), do: raise Error, message: "Unknown input type: #{inspect(type)}"


end

























