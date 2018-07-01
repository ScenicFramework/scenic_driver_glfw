#
#  Created by Boyd Multerer on 06/01/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#
# compile a graph out to a series of commands that can be drawn
# by the C render code
#
#


defmodule Scenic.Driver.Mac.Compile do
  alias Scenic.Primitive

  require Logger

  # import IEx

  #============================================================================
  # state handling
  @op_push_state              0x01
  @op_pop_state               0x02
  # @op_reset_state             0x03

  @op_run_script              0x04

  # render styles
  @op_paint_linear            0x06
  @op_paint_box               0x07
  @op_paint_radial            0x08
  @op_paint_image             0x09

  # @op_anti_alias              0x0A

  @op_stroke_width            0x0C
  @op_stroke_color            0x0D
  @op_stroke_paint            0x0E

  @op_fill_color              0x10
  @op_fill_paint              0x11

  @op_miter_limit             0x14
  @op_line_cap                0x15
  @op_line_join               0x16
  # @op_global_alpha            0x17

  # scissoring
  # @op_scissor                 0x1B
  @op_intersect_scissor       0x1C
  # @op_reset_scissor           0x1D

  # path operations
  @op_path_begin              0x20
  
  @op_path_move_to            0x21
  @op_path_line_to            0x22
  @op_path_bezier_to          0x23
  @op_path_quadratic_to       0x24
  @op_path_arc_to             0x25
  @op_path_close              0x26
  @op_path_winding            0x27
  
  @op_fill                    0x29
  @op_stroke                  0x2A
  
  @op_triangle                0x2C
  @op_arc                     0x2D
  @op_rect                    0x2E
  @op_round_rect              0x2F
  # @op_round_rect_var                  0x30
  @op_ellipse                 0x31
  @op_circle                  0x32
  @op_sector                  0x33

  @op_text                    0x34

  # transform operations
  # @op_tx_reset                0x36
  # @op_tx_identity             0x37
  @op_tx_matrix               0x38
  @op_tx_translate            0x39
  @op_tx_scale                0x3A
  @op_tx_rotate               0x3B
  @op_tx_skew_x               0x3C
  @op_tx_skew_y               0x3D

  # text/font styles
  @op_font                    0x40
  @op_font_blur               0x41
  @op_font_size               0x42
  @op_text_align              0x43
  @op_text_height             0x44

  @op_terminate               0xFF


  #============================================================================

  #--------------------------------------------------------
  def graph( nil, _, _ ) do
    []
  end
  def graph( _, nil, _ ) do
    []
  end
  def graph( graph, _graph_id, state ) do
    []
    |> compile_primitive( graph[0], graph, state )
    |> op_terminate()
    |> Enum.reverse()
  end


  #============================================================================
  # skip hidden primitives early
  defp compile_primitive( ops, %{styles: %{hidden: true}}, _, _ ) do
    ops
  end

  defp compile_primitive( ops, p, graph, state ) do
    ops
    |> op_push_state()
    |> compile_transforms( p )
    |> compile_styles( p )
    |> op_path_begin()
    |> do_compile_primitive( p, graph, state )
    |> do_fill( p )
    |> do_stroke( p )
    |> op_pop_state()
  end

  #--------------------------------------------------------
  defp do_fill(ops, %{styles: %{fill: paint}}) do
    case paint do
      {:image, {image, ox, oy, ex, ey, angle, alpha}} ->
        ops
        |> op_paint_image( image, ox, oy, ex, ey, angle, alpha )
        |> op_fill_paint()
      _ -> ops
    end
    |> op_fill()
  end
  defp do_fill(ops, _), do: ops

  #--------------------------------------------------------
  defp do_stroke(ops, %{styles: %{stroke: paint }}) do
    case paint do
      {:image, {image, ox, oy, ex, ey, angle, alpha}} ->
        ops
        |> op_paint_image( image, ox, oy, ex, ey, angle, alpha )
        |> op_stroke_paint()
      _ -> ops
    end
    |> op_stroke()
  end
  defp do_stroke(ops, _), do: ops

  #--------------------------------------------------------
  defp do_compile_primitive( ops, %{data: {Primitive.Group, ids}}, graph, state ) do
    Enum.reduce( ids, ops, fn(id, ops)->
      compile_primitive( ops, graph[id], graph, state )
    end)
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.Line, {{x0, y0}, {x1, y1}}}}, _, _
  ) do
    ops
    |> op_path_move_to(x0, y0)
    |> op_path_line_to(x1, y1)
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.Quad, {{x0, y0}, {x1, y1}, {x2, y2}, {x3, y3}}}}, _, _
  ) do
    ops
    |> op_path_move_to(x0, y0)
    |> op_path_line_to(x1, y1)
    |> op_path_line_to(x2, y2)
    |> op_path_line_to(x3, y3)
    |> op_path_close()
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.Triangle, {{x0, y0}, {x1, y1}, {x2, y2}}}}, _, _
  ) do
    ops
    |> op_triangle(x0, y0, x1, y1, x2, y2)
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.Rectangle, {width, height}}}, _, _
  ) do
    op_rect(ops, width, height)
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.RoundedRectangle, {width, height, radius}}}, _, _
  ) do
    op_round_rect( ops, width, height, radius )
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.Circle, radius}}, _, _
  ) do
    op_circle( ops, radius )
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.Ellipse, {r1, r2}}}, _, _
  ) do
    op_ellipse( ops, r1, r2 )
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.Arc, {radius, start, finish}} }, _, _
  ) do
    op_arc( ops, radius, start, finish )
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.Sector, {radius, start, finish}} }, _, _
  ) do
    op_sector( ops, radius, start, finish )
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.Path, actions}}, _, _ ) do
    Enum.reduce(actions, ops, fn
      :begin, ops -> op_path_begin(ops)
      {:move_to, x, y}, ops -> op_path_move_to(ops, x, y)
      {:line_to, x, y}, ops -> op_path_line_to(ops, x, y)
      {:bezier_to, c1x, c1y, c2x, c2y, x, y}, ops ->
        op_path_bezier_to(ops, c1x, c1y, c2x, c2y, x, y)
      {:quadratic_to, cx, cy, x, y}, ops ->
        op_path_quadratic_to(ops, cx, cy, x, y)
      {:arc_to, x1, y1, x2, y2, radius}, ops ->
        op_path_arc_to(ops, x1, y1, x2, y2, radius)
      :close_path, ops -> op_path_close(ops)
      :solid, ops -> op_path_winding(ops, :solid)
      :hole, ops -> op_path_winding(ops, :hole)
      # {:arc, cx, cy, r, a0, a1}, ops ->
      #   op_arc(ops, cx, cy, r, a0, a1, :solid)
      # {:rect, x, y, w, h}, ops -> op_rect(ops, x, y, w, h)
      # {:round_rect, x, y, w, h, r}, ops -> op_round_rect(ops, x, y, w, h, r)
      # {:ellipse, cx, cy, rx, ry}, ops -> op_ellipse(ops, cx, cy, rx, ry)
      # {:circle, cx, cy, r}, ops -> op_circle(ops, cx, cy, r)
    end)
  end

  defp do_compile_primitive( ops,
    %{data: {Primitive.Text, text}}, _, _
  ) do
    ops
    |> op_text( text )
  end


  defp do_compile_primitive( ops,
    %{data: {Primitive.SceneRef, {:graph,_,_} = graph_key}},
    _, %{dl_map: dl_map}
  ) do

    case dl_map[graph_key] do
      nil -> ops
      id ->
        [
          <<
            @op_run_script :: unsigned-integer-size(32)-native,
            id :: unsigned-integer-size(32)-native
          >>
          | ops
        ]
    end
  end


  # defp do_primitive_four( %{data: {Scenic.Primitive.SceneRef, {:graph,_,_} = graph_key}},
  # _, _, %{dl_map: dl_map} ) do

  #   case dl_map[graph_key] do
  #     nil ->
  #       # Logger.error "GLFW nil SceneRef #{inspect(graph_key)}"
  #       []

  #     dl_id ->
  #       # prepare the draw command
  #       <<
  #         @id_sub_graph :: size(8),
  #         dl_id :: unsigned-integer-size(32)-native
  #       >>
  #   end
  # end





  # ignore unrecognized primitives
  defp do_compile_primitive( ops, _p, _graph, _state ) do
   ops
  end



  #============================================================================
  defp compile_styles( ops, %{styles: styles} ) do
    Enum.reduce(styles, ops, fn({key, value}, ops) -> do_compile_style(ops, key, value) end)
  end
  defp compile_styles( ops, _ ), do: ops

  defp do_compile_style(ops, :join, type),          do: op_line_join(ops, type)
  defp do_compile_style(ops, :cap, type),           do: op_line_cap(ops, type)
  defp do_compile_style(ops, :miter_limit, limit),  do: op_miter_limit(ops, limit)

  defp do_compile_style(ops, :font, font),          do: op_font(ops, font)
  defp do_compile_style(ops, :font_blur, blur),     do: op_font_blur(ops, blur)
  defp do_compile_style(ops, :font_size, size),     do: op_font_size(ops, size)
  defp do_compile_style(ops, :text_align, align),   do: op_text_align(ops, align)
  defp do_compile_style(ops, :text_height, height), do: op_text_height(ops, height)

  defp do_compile_style(ops, :scissor, {{x,y}, w, h}) do
    [
      <<
        @op_intersect_scissor :: unsigned-integer-size(32)-native,
        x :: float-size(32)-native,
        y :: float-size(32)-native,
        w :: float-size(32)-native,
        h :: float-size(32)-native,
      >>
    | ops]
  end

  defp do_compile_style(ops, :fill, paint) do
    case paint do
      {:color, color} ->
        op_fill_color( ops, color )

      {:linear, {sx, sy, ex, ey, color_start, color_end}} ->
        ops
        |> op_paint_linear( sx, sy, ex, ey, color_start, color_end )
        |> op_fill_paint()

      {:box, {x, y, w, h, feather, radius, color_start, color_end}} ->
        ops
        |> op_paint_box( x, y, w, h, feather, radius, color_start, color_end )
        |> op_fill_paint()

      {:radial, {cx, cy, r_in, r_out, color_start, color_end}} ->
        ops
        |> op_paint_radial( cx, cy, r_in, r_out, color_start, color_end )
        |> op_fill_paint()

      # don't handle image pattern here. Should be set after the path is drawn

      _ -> ops
    end
  end

  defp do_compile_style(ops, :stroke, {width, paint}) do
    case paint do
      {:color, color} ->
        op_stroke_color( ops, color )

      {:linear, {sx, sy, ex, ey, color_start, color_end}} ->
        ops
        |> op_paint_linear( sx, sy, ex, ey, color_start, color_end )
        |> op_stroke_paint()

      {:box, {x, y, w, h, feather, radius, color_start, color_end}} ->
        ops
        |> op_paint_box( x, y, w, h, feather, radius, color_start, color_end )
        |> op_stroke_paint()

      {:radial, {cx, cy, r_in, r_out, color_start, color_end}} ->
        ops
        |> op_paint_radial( cx, cy, r_in, r_out, color_start, color_end )
        |> op_stroke_paint()

      # don't handle image pattern here. Should be set after the path is drawn

      _ -> ops
    end
    |> op_stroke_width( width )
  end

  defp do_compile_style(ops, _op, _value), do: ops

  #============================================================================
  defp compile_transforms( ops, %{transforms: txs} ) do
    ops
    |> do_compile_tx( :matrix, txs[:matrix] )
    |> do_compile_tx( :translate, txs[:translate] )
    |> do_compile_tx( :skew_x, txs[:skew_x] )
    |> do_compile_tx( :skew_y, txs[:skew_y] )
    |> do_compile_tx( :pin, txs[:pin] )
    |> do_compile_tx( :rotate, txs[:rotate] )
    |> do_compile_tx( :scale, txs[:scale] )
    |> do_compile_tx( :inv_pin, txs[:pin] )
  end
  defp compile_transforms( ops, _ ), do: ops

  defp do_compile_tx(ops, :translate, {dx,dy}) do
    [
      <<
        @op_tx_translate :: unsigned-integer-size(32)-native,
        dx :: float-size(32)-native,
        dy :: float-size(32)-native
      >>
    | ops]
  end

  defp do_compile_tx(ops, :pin, {dx,dy}) do
    [
      <<
        @op_tx_translate :: unsigned-integer-size(32)-native,
        dx :: float-size(32)-native,
        dy :: float-size(32)-native
      >>
    | ops]
  end

  defp do_compile_tx(ops, :inv_pin, {dx,dy}) do
    {dx,dy} = Scenic.Math.Vector.invert( {dx,dy} )
    [
      <<
        @op_tx_translate :: unsigned-integer-size(32)-native,
        dx :: float-size(32)-native,
        dy :: float-size(32)-native
      >>
    | ops]
  end

  defp do_compile_tx(ops, :scale, {sx,sy}) do
    [
      <<
        @op_tx_scale :: unsigned-integer-size(32)-native,
        sx :: float-size(32)-native,
        sy :: float-size(32)-native
      >>
    | ops]
  end

  defp do_compile_tx(ops, :rotate, value) when is_number(value) do
    [
      <<
        @op_tx_rotate :: unsigned-integer-size(32)-native,
        value :: float-size(32)-native,
      >>
    | ops]
  end

  defp do_compile_tx(ops, :skew_x, value) when is_number(value) do
    [
      <<
        @op_tx_skew_x :: unsigned-integer-size(32)-native,
        value :: float-size(32)-native,
      >>
    | ops]
  end

  defp do_compile_tx(ops, :skew_y, value) when is_number(value) do
    [
      <<
        @op_tx_skew_y :: unsigned-integer-size(32)-native,
        value :: float-size(32)-native,
      >>
    | ops]
  end

  defp do_compile_tx(ops, :matrix, <<
      a :: float-size(32)-native,
      c :: float-size(32)-native,
      e :: float-size(32)-native,
      b :: float-size(32)-native,
      d :: float-size(32)-native,
      f :: float-size(32)-native,
      _ :: binary
    >>) do
    [
      <<
        @op_tx_matrix :: unsigned-integer-size(32)-native,
        a :: float-size(32)-native,
        b :: float-size(32)-native,
        c :: float-size(32)-native,
        d :: float-size(32)-native,
        e :: float-size(32)-native,
        f :: float-size(32)-native
      >>
    | ops]
  end

  defp do_compile_tx(ops, _op, _v), do: ops


  #============================================================================
  # low-level commands that get compiled into the ops list
  # each new command is added to the front of the list, which will be reversed at the end.
 
  #--------------------------------------------------------
  defp op_push_state(ops),    do: [ <<@op_push_state :: unsigned-integer-size(32)-native >> | ops ]
  defp op_pop_state(ops),     do: [ <<@op_pop_state :: unsigned-integer-size(32)-native >> | ops ]
  # defp op_reset_state(ops),   do: [ <<@op_reset_state :: size(8) >> | ops ]

  # #--------------------------------------------------------
  # defp op_run_script(ops, script_id) do
  #   [
  #     <<
  #       @op_run_script :: size(8),
  #       script_id :: size(32)
  #     >>
  #   | ops]
  # end

  #--------------------------------------------------------
  defp op_paint_linear(ops, sx, sy, ex, ey, {sr, sg, sb, sa}, {er, eg, eb, ea}) do
    [
      <<
        @op_paint_linear :: unsigned-integer-size(32)-native,
        sx :: float-size(32)-native,
        sy :: float-size(32)-native,
        ex :: float-size(32)-native,
        ey :: float-size(32)-native,
        
        sr :: unsigned-integer-size(32)-native,
        sg :: unsigned-integer-size(32)-native,
        sb :: unsigned-integer-size(32)-native,
        sa :: unsigned-integer-size(32)-native,

        er :: unsigned-integer-size(32)-native,
        eg :: unsigned-integer-size(32)-native,
        eb :: unsigned-integer-size(32)-native,
        ea :: unsigned-integer-size(32)-native
      >>
    | ops]
  end

  defp op_paint_box(ops, x, y, w, h, radius, feather,
  {sr, sg, sb, sa}, {er, eg, eb, ea}) do
    [
      <<
        @op_paint_box :: unsigned-integer-size(32)-native,
        x :: float-size(32)-native,
        y :: float-size(32)-native,
        w :: float-size(32)-native,
        h :: float-size(32)-native,
        radius :: float-size(32)-native,
        feather :: float-size(32)-native,

        sr :: unsigned-integer-size(32)-native,
        sg :: unsigned-integer-size(32)-native,
        sb :: unsigned-integer-size(32)-native,
        sa :: unsigned-integer-size(32)-native,

        er :: unsigned-integer-size(32)-native,
        eg :: unsigned-integer-size(32)-native,
        eb :: unsigned-integer-size(32)-native,
        ea :: unsigned-integer-size(32)-native
      >>
    | ops]
  end

  defp op_paint_radial(ops, cx, cy, r_in, r_out,
  {sr, sg, sb, sa}, {er, eg, eb, ea}) do
    [
      <<
        @op_paint_radial :: unsigned-integer-size(32)-native,
        cx :: float-size(32)-native,
        cy :: float-size(32)-native,
        r_in :: float-size(32)-native,
        r_out :: float-size(32)-native,

        sr :: unsigned-integer-size(32)-native,
        sg :: unsigned-integer-size(32)-native,
        sb :: unsigned-integer-size(32)-native,
        sa :: unsigned-integer-size(32)-native,

        er :: unsigned-integer-size(32)-native,
        eg :: unsigned-integer-size(32)-native,
        eb :: unsigned-integer-size(32)-native,
        ea :: unsigned-integer-size(32)-native
      >>
    | ops]
  end

  defp op_paint_image(ops, image, ox, oy, ex, ey, angle, alpha) do
    name_size = byte_size(image) + 1

    # keep everything aligned on 4 byte boundaries
    {name_size, extra_buffer} = case 4 - rem( name_size, 4 ) do
      1 -> {name_size + 1, <<0 :: size(8)>>}
      2 -> {name_size + 2, <<0 :: size(16)>>}
      3 -> {name_size + 3, <<0 :: size(24)>>}
      _ -> {name_size, <<>>}
    end

    [
      <<
        @op_paint_image :: unsigned-integer-size(32)-native,
        ox :: float-size(32)-native,
        oy :: float-size(32)-native,
        ex :: float-size(32)-native,
        ey :: float-size(32)-native,
        angle :: float-size(32)-native,
        alpha :: unsigned-integer-size(32)-native,
        name_size :: unsigned-integer-size(32)-native,
        image :: binary,
        0 :: size(8), # null terminate to it can be used directly
        extra_buffer :: binary
      >>
    | ops]
  end


  #--------------------------------------------------------
  # defp op_anti_alias(ops, true) do
  #   [ << @op_anti_alias :: size(8), 1 :: size(8) >> | ops]
  # end
  # defp op_anti_alias(ops, false) do
  #   [ << @op_anti_alias :: size(8), 0 :: size(8) >> | ops]
  # end

  defp op_stroke_width(ops, w) do
    [
      <<
        @op_stroke_width :: unsigned-integer-size(32)-native,
        w :: float-size(32)-native
      >>
    | ops]
  end

  defp op_stroke_color(ops, {r, g, b, a}) do
    [
      <<
        @op_stroke_color :: unsigned-integer-size(32)-native,
        r :: unsigned-integer-size(32)-native,
        g :: unsigned-integer-size(32)-native,
        b :: unsigned-integer-size(32)-native,
        a :: unsigned-integer-size(32)-native
      >>
    | ops]
  end

  defp op_stroke_paint(ops) do
    [ << @op_stroke_paint :: unsigned-integer-size(32)-native >> | ops]
  end

  defp op_fill_color(ops, {r, g, b, a}) do
    [
      <<
        @op_fill_color :: unsigned-integer-size(32)-native,
        r :: unsigned-integer-size(32)-native,
        g :: unsigned-integer-size(32)-native,
        b :: unsigned-integer-size(32)-native,
        a :: unsigned-integer-size(32)-native
      >>
    | ops]
  end

  defp op_fill_paint(ops) do
    [ << @op_fill_paint :: unsigned-integer-size(32)-native >> | ops]
  end

  defp op_miter_limit(ops, limit) do
    [
      <<
        @op_miter_limit :: unsigned-integer-size(32)-native,
        limit :: float-size(32)-native
      >>
    | ops]
  end

  defp op_line_cap(ops, :butt) do
    [
      <<
        @op_line_cap :: unsigned-integer-size(32)-native,
        0 :: unsigned-integer-size(32)-native
      >>
    | ops]
  end
  defp op_line_cap(ops, :round) do
    [
      <<
        @op_line_cap :: unsigned-integer-size(32)-native,
        1 :: unsigned-integer-size(32)-native
      >>
    | ops]
  end
  defp op_line_cap(ops, :square) do
    [
      <<
        @op_line_cap :: unsigned-integer-size(32)-native, 
        2 :: unsigned-integer-size(32)-native
      >>
    | ops]
  end

  defp op_line_join(ops, :miter) do
    [
      <<
        @op_line_join :: unsigned-integer-size(32)-native,
        0 :: unsigned-integer-size(32)-native
      >>
    | ops]
  end
  defp op_line_join(ops, :round) do
    [
      <<
        @op_line_join :: unsigned-integer-size(32)-native,
        1 :: unsigned-integer-size(32)-native
      >>
    | ops]
  end
  defp op_line_join(ops, :bevel) do
    [
      <<
        @op_line_join :: unsigned-integer-size(32)-native,
        2 :: unsigned-integer-size(32)-native
      >>
    | ops]
  end

  defp op_font(ops, font) do
    font_name = to_string(font)
    name_size = byte_size(font_name) + 1

    # keep everything aligned on 4 byte boundaries
    {name_size, extra_buffer} = case 4 - rem( name_size, 4 ) do
      1 -> {name_size + 1, <<0 :: size(8)>>}
      2 -> {name_size + 2, <<0 :: size(16)>>}
      3 -> {name_size + 3, <<0 :: size(24)>>}
      _ -> {name_size, <<>>}
    end

    [
      <<
        @op_font :: unsigned-integer-size(32)-native,
        name_size :: unsigned-integer-size(32)-native,
        font_name :: binary,
        # null terminate the name here. This allows us to use the
        # string in the script directly without copying it to a new buffer
        0 :: size(8),
        extra_buffer :: binary
      >>
    | ops]
  end

  defp op_font_blur(ops, blur) do
    [
      <<
        @op_font_blur :: unsigned-integer-size(32)-native,
        blur :: float-size(32)-native
      >>
    | ops]
  end
  defp op_font_size(ops, point_size) do
    [
      <<
        @op_font_size :: unsigned-integer-size(32)-native,
        point_size :: float-size(32)-native
      >>
    | ops]
  end

  defp op_text_align(ops, :left), do: op_text_align(ops, 0b1000001)
  defp op_text_align(ops, :center), do: op_text_align(ops, 0b1000010)
  defp op_text_align(ops, :right), do: op_text_align(ops, 0b1000100)
  defp op_text_align(ops, :left_top), do: op_text_align(ops, 0b0001001)
  defp op_text_align(ops, :center_top), do: op_text_align(ops, 0b0001010)
  defp op_text_align(ops, :right_top), do: op_text_align(ops, 0b0001100)
  defp op_text_align(ops, :left_middle), do: op_text_align(ops, 0b0010001)
  defp op_text_align(ops, :center_middle), do: op_text_align(ops, 0b0010010)
  defp op_text_align(ops, :right_middle), do: op_text_align(ops, 0b0010100)
  defp op_text_align(ops, :left_bottom), do: op_text_align(ops, 0b0100001)
  defp op_text_align(ops, :center_bottom), do: op_text_align(ops, 0b0100010)
  defp op_text_align(ops, :right_bottom), do: op_text_align(ops, 0b0100100)
  defp op_text_align(ops, flags) when is_integer(flags) do
    [
      <<
        @op_text_align :: unsigned-integer-size(32)-native,
        flags :: unsigned-integer-size(32)-native
      >>
    | ops]
  end


  defp op_text_height(ops, height) do
    [
      <<
        @op_text_height :: unsigned-integer-size(32)-native,
        height :: float-size(32)-native
      >>
    | ops]
  end




  #--------------------------------------------------------
  defp op_path_begin(ops),    do: [ <<@op_path_begin :: unsigned-integer-size(32)-native >> | ops]
  defp op_path_move_to(ops, x, y) do
    [
      <<
        @op_path_move_to :: unsigned-integer-size(32)-native,
        x :: float-size(32)-native,
        y :: float-size(32)-native
      >>
    | ops]
  end
  defp op_path_line_to(ops, x, y) do
    [
      <<
        @op_path_line_to :: unsigned-integer-size(32)-native,
        x :: float-size(32)-native,
        y :: float-size(32)-native
      >>
    | ops]
  end
  defp op_path_bezier_to(ops, c1x, c1y, c2x, c2y, x, y) do
    [
      <<
        @op_path_bezier_to :: unsigned-integer-size(32)-native,
        c1x :: float-size(32)-native,
        c1y :: float-size(32)-native,
        c2x :: float-size(32)-native,
        c2y :: float-size(32)-native,
        x   :: float-size(32)-native,
        y   :: float-size(32)-native
      >>
    | ops]
  end
  defp op_path_quadratic_to(ops, cx, cy, x, y) do
    [
      <<
        @op_path_quadratic_to :: unsigned-integer-size(32)-native,
        cx :: float-size(32)-native,
        cy :: float-size(32)-native,
        x   :: float-size(32)-native,
        y   :: float-size(32)-native
      >>
    | ops]
  end
  defp op_path_arc_to(ops, x1, y1, x2, y2, radius) do
    [
      <<
        @op_path_arc_to :: unsigned-integer-size(32)-native,
        x1 :: float-size(32)-native,
        y1 :: float-size(32)-native,
        x2 :: float-size(32)-native,
        y2 :: float-size(32)-native,
        radius :: float-size(32)-native
      >>
    | ops]
  end
  defp op_path_close(ops) do
    [ << @op_path_close :: unsigned-integer-size(32)-native >> | ops]
  end
  defp op_path_winding(ops, :solid) do
    [
      <<
        @op_path_winding :: unsigned-integer-size(32)-native,
        1 :: unsigned-integer-size(32)-native,
      >>
    | ops]
  end
  defp op_path_winding(ops, :hole) do
    [
      <<
        @op_path_winding :: unsigned-integer-size(32)-native,
        0 :: unsigned-integer-size(32)-native,
      >>
    | ops]
  end

  #--------------------------------------------------------
  defp op_triangle(ops, x0, y0, x1, y1, x2, y2) do
    [
      <<
        @op_triangle :: unsigned-integer-size(32)-native,
        x0 :: float-size(32)-native,
        y0 :: float-size(32)-native,
        x1 :: float-size(32)-native,
        y1 :: float-size(32)-native,
        x2 :: float-size(32)-native,
        y2 :: float-size(32)-native,
      >>
    | ops]
  end


  defp op_rect(ops, w, h) do
    [
      <<
        @op_rect :: unsigned-integer-size(32)-native,
        w :: float-size(32)-native,
        h :: float-size(32)-native
      >>
    | ops]
  end

  defp op_round_rect(ops, w, h, r) do
    [
      <<
        @op_round_rect :: unsigned-integer-size(32)-native,
        w :: float-size(32)-native,
        h :: float-size(32)-native,
        r :: float-size(32)-native
      >>
    | ops]
  end

  defp op_circle(ops, r) do
    [
      <<
        @op_circle :: unsigned-integer-size(32)-native,
        r :: float-size(32)-native
      >>
    | ops]
  end

  # defp op_arc(ops,_,_,_, start, finish, _,_) when start == finish, do: ops
  defp op_arc(ops, r, start, finish ) do
    [
      <<
        @op_arc :: unsigned-integer-size(32)-native,
        r :: float-size(32)-native,
        start :: float-size(32)-native,
        finish :: float-size(32)-native
      >>
    | ops]
  end

  # defp op_sector(ops,_,_,_, start, finish, _,_) when start == finish, do: ops
  defp op_sector(ops, r, start, finish ) do
    [
      <<
        @op_sector :: unsigned-integer-size(32)-native,
        r :: float-size(32)-native,
        start :: float-size(32)-native,
        finish :: float-size(32)-native
      >>
    | ops]
  end


  defp op_ellipse(ops, rx, ry) do
    [
      <<
        @op_ellipse :: unsigned-integer-size(32)-native,
        rx :: float-size(32)-native,
        ry :: float-size(32)-native
      >>
    | ops]
  end

  defp op_text(ops, text) do
    text_size = byte_size(text) + 1

    # keep everything aligned on 4 byte boundaries
    {text_size, extra_buffer} = case 4 - rem( text_size, 4 ) do
      1 -> {text_size + 1, <<0 :: size(8)>>}
      2 -> {text_size + 2, <<0 :: size(16)>>}
      3 -> {text_size + 3, <<0 :: size(24)>>}
      _ -> {text_size, <<>>}
    end

    [
      <<
        @op_text :: unsigned-integer-size(32)-native,
        text_size :: unsigned-integer-size(32)-native,
        text :: binary,
        0 :: size(8), # null terminate the string so it can be used directly
        extra_buffer :: binary
      >>
    | ops]
  end



  #--------------------------------------------------------
  defp op_fill(ops),    do: [ <<@op_fill :: unsigned-integer-size(32)-native >> | ops]
  defp op_stroke(ops),  do: [ <<@op_stroke :: unsigned-integer-size(32)-native >> | ops]






  #--------------------------------------------------------
  defp op_terminate(ops), do: [ <<@op_terminate :: unsigned-integer-size(32)-native >> | ops]

end



























