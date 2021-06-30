defmodule Scenic.Driver.Glfw.ToPort do
  use Bitwise

  @cmd_put_script 0x01
  @cmd_del_script 0x02
  @cmd_reset_scripts 0x03

  @cmd_render 0x06

  @cmd_request_input 0x0A

  @cmd_close 0x20
  # @cmd_query_stats 0x21
  @cmd_reshape 0x22
  @cmd_position 0x23
  @cmd_focus 0x24
  @cmd_iconify 0x25
  @cmd_maximize 0x26
  @cmd_restore 0x27
  @cmd_show 0x28
  @cmd_hide 0x29

  @cmd_put_font 0x40
  @cmd_put_img 0x41

  @min_window_width 40
  @min_window_height 20



  @input_key  0x01
  @input_codepoint  0x02
  @input_cursor_pos  0x04
  @input_cursor_button  0x08
  @input_cursor_scroll  0x10
  @input_cursor_enter  0x20


  def put_script( script, id, port ) do
    msg = [
      <<
        @cmd_put_script::unsigned-integer-size(32)-native,
        id::integer-size(32)-native,
      >>,
      script
    ]

    Port.command(port, msg)
  end

  def del_script( id, port ) do
    msg = <<
      @cmd_del_script::unsigned-integer-size(32)-native,
      id::integer-size(32)-big
    >>

    Port.command(port, msg)
  end

  def reset_start( port ) do
    Port.command(port, <<@cmd_reset_scripts::unsigned-integer-size(32)-native>>)
  end

  def render( port ) do
    Port.command(port, <<@cmd_render::unsigned-integer-size(32)-native>>)
  end

  def request_inputs( inputs, port ) when is_list(inputs) do
    input_flags = Enum.reduce( inputs, 0, fn
      :key, flags -> flags ||| @input_key
      :codepoint, flags -> flags ||| @input_codepoint
      :cursor_pos, flags -> flags ||| @input_cursor_pos
      :cursor_button, flags -> flags ||| @input_cursor_button
      :cursor_scroll, flags -> flags ||| @input_cursor_scroll
      :viewport, flags -> flags ||| @input_cursor_enter
      _, flags -> flags
    end)

    msg = <<
      @cmd_request_input::unsigned-integer-size(32)-native,
      input_flags::integer-size(32)-native
    >>

    Port.command(port, msg)
  end

  def reshape( width, height, port)  when is_integer(width) and is_integer(height) do
    # enforce a minimum size...
    w =
      cond do
        width < @min_window_width -> @min_window_width
        true -> width
      end

    h =
      cond do
        height < @min_window_height -> @min_window_height
        true -> height
      end

    msg = <<
      @cmd_reshape::unsigned-integer-size(32)-native,
      w::integer-size(32)-native,
      h::integer-size(32)-native
    >>

    Port.command(port, msg)
  end

  def position( x, y, port)  when is_integer(x) and is_integer(y) do
    msg = <<
      @cmd_position::unsigned-integer-size(32)-native,
      x::integer-size(32)-native,
      y::integer-size(32)-native
    >>

    Port.command(port, msg)
  end

  def close( port ) do
    Port.command(port, <<@cmd_close::unsigned-integer-size(32)-native>>)
  end

  def focus( port ) do
    Port.command(port, <<@cmd_focus::unsigned-integer-size(32)-native>>)
  end

  def iconify( port ) do
    Port.command(port, <<@cmd_iconify::unsigned-integer-size(32)-native>>)
  end

  def maximize( port ) do
    Port.command(port, <<@cmd_maximize::unsigned-integer-size(32)-native>>)
  end

  def restore( port ) do
    Port.command(port, <<@cmd_restore::unsigned-integer-size(32)-native>>)
  end

  def show( port ) do
    Port.command(port, <<@cmd_show::unsigned-integer-size(32)-native>>)
  end

  def hide( port ) do
    Port.command(port, <<@cmd_hide::unsigned-integer-size(32)-native>>)
  end

  def put_font( port, name, bin ) when is_binary(name) and is_binary(bin) do
    name_bytes = byte_size(name)
    msg = [
      <<
        @cmd_put_font::unsigned-integer-size(32)-native,
        name_bytes::unsigned-integer-size(32)-native
      >>,
      name,
      bin
    ]

    Port.command(port, msg)
  end

  def put_texture( port, id, format, w, h, bin )

  def put_texture( _port, _id, _kind, _w, _h, nil ) do
  end

  def put_texture( port, id, :file, w, h, bin ) do
    do_put_texture( port, id, 0, w, h, bin )
  end

  def put_texture( port, id, :g, w, h, bin ) do
    do_put_texture( port, id, 1, w, h, bin )
  end

  def put_texture( port, id, :ga, w, h, bin ) do
    do_put_texture( port, id, 2, w, h, bin )
  end

  def put_texture( port, id, :rgb, w, h, bin ) do
    do_put_texture( port, id, 3, w, h, bin )
  end

  def put_texture( port, id, :rgba, w, h, bin ) do
    do_put_texture( port, id, 4, w, h, bin )
  end

  def do_put_texture( port, <<_::binary-size(32)>> = id, format, w, h, bin )
  when is_integer(w) and is_integer(h) and is_binary(bin) do
    msg = [
      << @cmd_put_img::unsigned-integer-size(32)-native >>,
      id,
      <<
        w::unsigned-integer-size(32)-native,
        h::unsigned-integer-size(32)-native,
        format::unsigned-integer-size(32)-native
      >>,
      bin
    ]
    Port.command(port, msg)
    render( port )
  end


end