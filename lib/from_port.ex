defmodule Scenic.Driver.Glfw.FromPort do
  alias Scenic.ViewPort
  alias Scenic.Driver

  # import IEx

  require Logger

  import Driver,
    only: [
      assign: 2,
      assign: 3,
      set_busy: 2,
      send_input: 2
    ]

  # incoming message ids
  @msg_close_id 0x00
  #  @msg_stats_id             0x01
  @msg_puts_id 0x02
  @msg_write_id 0x03
  @msg_inspect_id 0x04
  @msg_reshape_id 0x05
  @msg_ready_id 0x06
  # @msg_draw_ready_id 0x07

  @msg_key_id 0x0A
  @msg_char_id 0x0B
  @msg_cursor_pos_id 0x0C
  @msg_mouse_button_id 0x0D
  @msg_mouse_scroll_id 0x0E
  @msg_cursor_enter_id 0x0F

  # @msg_static_texture_miss 0x20
  # @msg_dynamic_texture_miss 0x21

  # @msg_font_miss 0x22
  # @msg_texture_miss 0x23

  # ============================================================================

  @doc false
  def handle_port_message(msg, state)

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_ready_id::unsigned-integer-size(32)-native
        >>,
        driver
      ) do
    {:noreply, set_busy(driver, false)}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_reshape_id::unsigned-integer-size(32)-native,
          window_width::unsigned-integer-size(32)-native,
          window_height::unsigned-integer-size(32)-native,
          frame_width::unsigned-integer-size(32)-native,
          frame_height::unsigned-integer-size(32)-native
        >>,
        driver
      ) do
    driver =
      assign(driver,
        window: {window_width, window_height},
        frame: {frame_width, frame_height},
        screen_factor: {frame_width / window_width}
      )
      |> send_input({:viewport, {:reshape, {window_width, window_height}}})
      |> Driver.request_update()

    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_close_id::unsigned-integer-size(32)-native,
          reason::unsigned-integer-size(32)-native
        >>,
        %{assigns: %{on_close: on_close}, viewport: viewport} = driver
      ) do
    case on_close do
      :stop_driver -> ViewPort.stop_driver(viewport, self())
      :stop_viewport -> ViewPort.stop(viewport)
      :stop_system -> System.stop(reason)
      :halt_system -> System.halt(reason)
      {module, _fun, 1} -> module.fun(reason)
    end

    {:noreply, assign(driver, :closing, true)}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<@msg_puts_id::unsigned-integer-size(32)-native>> <> msg,
        driver
      ) do
    IO.inspect(msg, label: "glfw_puts")
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<@msg_write_id::unsigned-integer-size(32)-native>> <> msg,
        driver
      ) do
    IO.write(msg)
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<@msg_inspect_id::unsigned-integer-size(32)-native>> <> msg,
        driver
      ) do
    IO.inspect(msg)
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_key_id::unsigned-integer-size(32)-native,
          key::unsigned-integer-native-size(32),
          _scancode::unsigned-integer-native-size(32),
          action::unsigned-integer-native-size(32),
          mods::unsigned-integer-native-size(32)
        >>,
        driver
      ) do
    key = key_to_atom(key)
    # action = action_to_atom(action)
    mods = prep_mods(mods)
    send_input(driver, {:key, {key, action, mods}})

    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_char_id::unsigned-integer-size(32)-native,
          codepoint::unsigned-integer-native-size(32),
          mods::unsigned-integer-native-size(32)
        >>,
        driver
      ) do
    codepoint = codepoint_to_char(codepoint)
    mods = prep_mods(mods)
    send_input(driver, {:codepoint, {codepoint, mods}})

    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_cursor_pos_id::unsigned-integer-size(32)-native,
          x::float-native-size(32),
          y::float-native-size(32)
        >>,
        # state
        driver
      ) do
    send_input(driver, {:cursor_pos, {x, y}})
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_mouse_button_id::unsigned-integer-size(32)-native,
          button::unsigned-integer-native-size(32),
          action::unsigned-integer-native-size(32),
          mods::unsigned-integer-native-size(32),
          x::float-native-size(32),
          y::float-native-size(32)
        >>,
        driver
      ) do
    # action = action_to_atom(action)
    button = button_to_atom(button)
    mods = prep_mods(mods)
    send_input(driver, {:cursor_button, {button, action, mods, {x, y}}})
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_mouse_scroll_id::unsigned-integer-size(32)-native,
          x_offset::float-native-size(32),
          y_offset::float-native-size(32),
          x_pos::float-native-size(32),
          y_pos::float-native-size(32)
        >>,
        driver
      ) do
    input = {:cursor_scroll, {{x_offset, y_offset}, {x_pos, y_pos}}}
    send_input(driver, input)
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_cursor_enter_id::unsigned-integer-size(32)-native,
          0::integer-native-size(32),
          x_pos::float-native-size(32),
          y_pos::float-native-size(32)
        >>,
        driver
      ) do
    send_input(driver, {:viewport, {:exit, {x_pos, y_pos}}})
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_cursor_enter_id::unsigned-integer-size(32)-native,
          1::integer-native-size(32),
          x_pos::float-native-size(32),
          y_pos::float-native-size(32)
        >>,
        driver
      ) do
    send_input(driver, {:viewport, {:enter, {x_pos, y_pos}}})
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<id::unsigned-integer-size(32)-native, bin::binary>>,
        driver
      ) do
    IO.puts("Unhandled port messages id: #{id}, msg: #{inspect(bin)}")
    {:noreply, driver}
  end

  # ============================================================================
  # utilities to translate Glfw input to standardized input

  @glfw_button_atoms %{
    0 => :btn_left,
    1 => :btn_right,
    2 => :btn_middle
  }
  defp button_to_atom(code), do: Map.get(@glfw_button_atoms, code, :unknown)

  # ============================================================================
  # keyboard input helpers
  # these are for reading the keyboard directly. If you are trying to do text input
  # use the text/char helpers instead

  # key codes use the standards defined by Glfw
  # http://www.Glfw.org/docs/latest/group__keys.html

  # --------------------------------------------------------
  @glfw_key_atoms %{
    32 => :key_space,
    39 => :key_apostrophe,
    44 => :key_comma,
    45 => :key_minus,
    46 => :key_dot,
    47 => :key_slash,
    48 => :key_0,
    49 => :key_1,
    50 => :key_2,
    51 => :key_3,
    52 => :key_4,
    53 => :key_5,
    54 => :key_6,
    55 => :key_7,
    56 => :key_8,
    57 => :key_9,
    59 => :key_semicolon,
    61 => :key_equal,
    65 => :key_a,
    66 => :key_b,
    67 => :key_c,
    68 => :key_d,
    69 => :key_e,
    70 => :key_f,
    71 => :key_g,
    72 => :key_h,
    73 => :key_i,
    74 => :key_j,
    75 => :key_k,
    76 => :key_l,
    77 => :key_m,
    78 => :key_n,
    79 => :key_o,
    80 => :key_p,
    81 => :key_q,
    82 => :key_r,
    83 => :key_s,
    84 => :key_t,
    85 => :key_u,
    86 => :key_v,
    87 => :key_w,
    88 => :key_x,
    89 => :key_y,
    90 => :key_z,
    91 => :key_leftbrace,
    92 => :key_backslash,
    93 => :key_rightbrace,
    96 => :key_grave,
    256 => :key_esc,
    257 => :key_enter,
    258 => :key_tab,
    259 => :key_backspace,
    260 => :key_insert,
    261 => :key_delete,
    262 => :key_right,
    263 => :key_left,
    264 => :key_down,
    265 => :key_up,
    266 => :key_pageup,
    267 => :key_pagedown,
    268 => :key_home,
    269 => :key_end,
    280 => :key_capslock,
    281 => :key_scrolllock,
    282 => :key_numlock,
    283 => :key_screen,
    284 => :key_pause,
    290 => :key_f1,
    291 => :key_f2,
    292 => :key_f3,
    293 => :key_f4,
    294 => :key_f5,
    295 => :key_f6,
    296 => :key_f7,
    297 => :key_f8,
    298 => :key_f9,
    299 => :key_f10,
    300 => :key_f11,
    301 => :key_f12,
    302 => :key_f13,
    303 => :key_f14,
    304 => :key_f15,
    305 => :key_f16,
    306 => :key_f17,
    307 => :key_f18,
    308 => :key_f19,
    309 => :key_f20,
    310 => :key_f21,
    311 => :key_f22,
    312 => :key_f23,
    313 => :key_f24,
    314 => :key_f25,
    320 => :key_kp0,
    321 => :key_kp1,
    322 => :key_kp2,
    323 => :key_kp3,
    324 => :key_kp4,
    325 => :key_kp5,
    326 => :key_kp6,
    327 => :key_kp7,
    328 => :key_kp8,
    329 => :key_kp9,
    330 => :key_kpdot,
    331 => :key_kpslash,
    332 => :key_kpasterisk,
    333 => :key_kpminus,
    334 => :key_kpplus,
    335 => :key_kpenter,
    336 => :key_kpequal,
    340 => :key_leftshift,
    341 => :key_leftctrl,
    342 => :key_leftalt,
    # 343 => "left_super"

    344 => :key_rightshift,
    345 => :key_rightctrl,
    346 => :key_rightalt,
    # 347 => "right_super"

    348 => :key_menu
  }
  defp key_to_atom(code), do: Map.get(@glfw_key_atoms, code, :unknown)

  # --------------------------------------------------------
  @glfw_mod_shift 0x001
  @glfw_mod_ctrl 0x002
  @glfw_mod_alt 0x004
  @glfw_mod_super 0x008
  @glfw_mod_caps_lock 0x010
  @glfw_mod_num_lock 0x020
  defp prep_mods(mods) do
    []
    |> add_if_masked(mods, @glfw_mod_shift, :shift)
    |> add_if_masked(mods, @glfw_mod_ctrl, :ctrl)
    |> add_if_masked(mods, @glfw_mod_alt, :alt)
    |> add_if_masked(mods, @glfw_mod_super, :meta)
    |> add_if_masked(mods, @glfw_mod_caps_lock, :caps_lock)
    |> add_if_masked(mods, @glfw_mod_num_lock, :num_lock)
  end

  defp add_if_masked(list, mods, mask, key) do
    case Bitwise.&&&(mods, mask) do
      0 -> list
      _ -> [key | list]
    end
  end

  # --------------------------------------------------------
  defp codepoint_to_char(codepoint_to_atom)
  defp codepoint_to_char(cp), do: <<cp::utf8>>
end
