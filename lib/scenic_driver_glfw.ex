defmodule Scenic.Driver.Glfw do
  @moduledoc """
  Documentation for `Scenic.Driver.Glfw`.
  """
  use Scenic.Driver
  require Logger

  alias Scenic.Script
  alias Scenic.ViewPort
  alias Scenic.Driver.Glfw
  alias Scenic.Assets.Static
  alias Scenic.Assets.Stream

  # import IEx

  @driver_ext if elem(:os.type(), 0) == :win32, do: '.exe', else: ''
  @port '/' ++ to_charlist(Mix.env()) ++ '/scenic_driver_glfw' ++ @driver_ext

  # default values
  @default_title "Driver Glfw"
  @default_sync 15

  @opts_schema [
    name: [type: {:or, [:atom, :string]}],
    title: [type: :string, default: @default_title],
    sync_ms: [type: :integer, default: @default_sync],
    resizeable: [type: :boolean, default: false],
    on_close: [
      type: {:or, [:mfa, {:in, [:stop_driver, :stop_viewport, :stop_system, :halt_system]}]},
      default: :stop_system
    ]
  ]

  @debounce_ms 15

  # same as scenic/script.ex
  @op_font 0x90
  @op_fill_image 0x63
  @op_stroke_image 0x74

  def validate_opts(opts), do: NimbleOptions.validate(opts, @opts_schema)

  # --------------------------------------------------------
  def init(viewport, opts) do
    # device_id = "abc"
    {width, height} = viewport.size

    IO.puts("================> INIT GLFW DRIVER #{inspect(self())} <================")

    # prepare the agr string to start the port exe
    port_args = " #{width} #{height} #{opts[:resizeable]} \"#{opts[:title]}\""

    # open and initialize the window
    Process.flag(:trap_exit, true)
    executable = :code.priv_dir(:scenic_driver_glfw) ++ @port ++ to_charlist(port_args)
    port = Port.open({:spawn, executable}, [:binary, {:packet, 4}])

    state = %{
      port: port,
      screen_factor: 1.0,
      size: {width, height},
      closing: false,
      debounce: %{},
      on_close: opts[:on_close],
      media: %{},
      dirty_ids: [],
      viewport: viewport,
      opts: opts,
      gated: false,
      debounce_scripts: false,
      debounce_redraw: false,
      debounce_ms: @debounce_ms,
      busy: true
    }

    {:ok, state}
  end

  # --------------------------------------------------------
  @doc false
  def handle_call(_msg, _from, %{} = state) do
    {:reply, {:error, :not_implemented}, state}
  end

  # --------------------------------------------------------
  @doc false

  # handle input requests
  # def handle_cast( {:request_input, inputs}, state ) do
  def handle_cast({:request_input, inputs}, %{port: port} = state) do
    Glfw.ToPort.request_inputs(inputs, port)
    {:noreply, state}
  end

  # --------------------------------------------------------
  # reset is received when the root scene is being changed
  # Reset the tracked meda
  def handle_cast(:reset, %{port: port} = state) do
    Stream.unsubscribe(:all)
    Glfw.ToPort.reset_start(port)

    # state changes
    state =
      state
      |> Map.put(:dirty_ids, [0])
      |> Map.put(:debounce_scripts, false)
      |> Map.put(:busy, false)
      |> Map.put(:media, %{})

    # reset tracked media
    {:noreply, state}
  end

  # --------------------------------------------------------
  # A gate signal is sent as scenes are staring up and complete is sent when it is done,
  # including any child scenes.
  def handle_cast(:gate_start, %{gated: false} = state) do
    # state changes
    state =
      state
      |> Map.put(:debounce_scripts, false)
      |> Map.put(:busy, false)
      |> Map.put(:gated, true)

    {:noreply, state}
  end

  def handle_cast(:gate_start, state), do: {:noreply, state}

  # the reset has completed
  def handle_cast(:gate_complete, %{gated: true, port: port} = state) do
    Glfw.ToPort.render(port)
    {:noreply, %{state | gated: false}}
  end

  # --------------------------------------------------------
  # put scripts

  # if we are in a gate, just send the script right away, no need or desire to debounce them
  def handle_cast({:put_scripts, ids}, %{dirty_ids: dirty, gated: true} = state) do
    state = do_put_scripts([ids | dirty], state)
    {:noreply, %{state | dirty_ids: []}}
  end

  # if debouncing, store the script ids instead of sending them right away
  def handle_cast({:put_scripts, ids}, %{dirty_ids: dirty_ids, debounce_scripts: true} = state) do
    # IO.write "."
    {:noreply, %{state | dirty_ids: [ids | dirty_ids]}}
  end

  # if busy, story the script instead of sending it right away
  def handle_cast({:put_scripts, ids}, %{dirty_ids: dirty_ids, busy: true} = state) do
    {:noreply, %{state | dirty_ids: [ids | dirty_ids]}}
  end

  # not debouncing. Send the ids right away and start new debounce_scripts
  def handle_cast(
        {:put_scripts, ids},
        %{dirty_ids: dirty_ids, port: port} = state
      ) do
    state = do_put_scripts([ids | dirty_ids], state)
    Glfw.ToPort.render(port)
    {:noreply, state}
  end

  # --------------------------------------------------------
  @doc false

  # Debounce time is up. No scripts arrived, so stop debouncing.
  def handle_info(:debounce_scripts, %{dirty_ids: []} = state) do
    {:noreply, %{state | debounce_scripts: false}}
  end

  # Debounce time is up. Scripts did arrive, so debounce_scripts again.
  def handle_info(:debounce_scripts, %{dirty_ids: ids, port: port} = state) do
    state = do_put_scripts(ids, state)
    Glfw.ToPort.render(port)
    {:noreply, state}
  end

  # --------------------------------------------------------
  # deal with the app exiting normally
  def handle_info({:EXIT, port_id, :normal}, %{port: port, closing: closing} = state)
      when port_id == port do
    if closing do
      Logger.info("clean close")
      # we are closing cleanly, let it happen.
      GenServer.stop(self())
      {:noreply, state}
    else
      Logger.error("dirty close")
      # we are not closing cleanly. Let the supervisor recover.
      {:noreply, state}
    end
  end

  # --------------------------------------------------------
  # def handle_info({:debounce, type}, state) do
  #   Glfw.FromPort.handle_debounce(type, state)
  # end

  # --------------------------------------------------------
  # streaming asset updates
  def handle_info({{Stream, :put}, Stream.Image, id}, %{port: port} = state) do
    with {:ok, {Stream.Bitmap, {w, h, _mime}, bin}} <- Stream.fetch(id) do
      id32 = gen_id32_from_string(id)
      Glfw.ToPort.put_texture(port, id32, :file, w, h, bin)
    end

    {:noreply, state}
  end

  def handle_info({{Stream, :put}, Stream.Bitmap, id}, %{port: port} = state) do
    with {:ok, {Stream.Bitmap, {w, h, type}, bin}} <- Stream.fetch(id) do
      id32 = gen_id32_from_string(id)
      Glfw.ToPort.put_texture(port, id32, type, w, h, bin)
    end

    {:noreply, state}
  end

  def handle_info({{Stream, _verb}, _type, _id}, state), do: {:noreply, state}

  # --------------------------------------------------------
  # messages from the port
  def handle_info({pid, {:data, data}}, %{port: port} = state) when pid == port do
    Glfw.FromPort.handle_port_message(data, state)
  end

  # --------------------------------------------------------
  defp do_put_scripts(ids, %{viewport: vp, port: port} = state) do
    ids
    |> List.flatten()
    |> Enum.uniq()
    |> Enum.sort()
    |> case do
      [] ->
        state

      ids ->
        Enum.reduce(ids, state, fn id, state ->
          with {:ok, script} <- ViewPort.get_script_by_id(vp, id) do
            state = ensure_media(script, state)

            Script.serialize(script, fn
              {:font, id} -> serialize_font(id)
              {:fill_stream, id} -> serialize_fill_stream(id)
              {:stroke_stream, id} -> serialize_stroke_stream(id)
              other -> other
            end)
            |> Glfw.ToPort.put_script(id, port)

            state
          else
            _ -> state
          end
        end)
    end
  end

  defp ensure_media(script, state) do
    media = Script.media(script, :id)

    state
    |> ensure_fonts(Map.get(media, :fonts, []))
    |> ensure_images(Map.get(media, :images, []))
    |> ensure_streams(Map.get(media, :streams, []))
  end

  defp ensure_fonts(state, []), do: state

  defp ensure_fonts(%{port: port} = state, ids) do
    fonts = Map.get(state.media, :fonts, [])

    fonts =
      Enum.reduce(ids, fonts, fn id, fonts ->
        with false <- Enum.member?(fonts, id),
             {:ok, {Static.Font, _}} <- Static.fetch(id),
             {:ok, _, hash} <- Static.to_hash(id),
             {:ok, bin} <- Static.load(id) do
          Glfw.ToPort.put_font(port, hash, bin)
          [id | fonts]
        else
          _ -> fonts
        end
      end)

    put_in(state, [:media, :fonts], fonts)
  end

  defp ensure_images(state, []), do: state

  defp ensure_images(%{port: port, media: media} = state, ids) do
    images = Map.get(media, :images, [])

    images =
      Enum.reduce(ids, images, fn id, images ->
        with false <- Enum.member?(images, id),
             {:ok, {Static.Image, {w, h, _}}} <- Static.fetch(id),
             {:ok, hash, _} <- Static.to_hash(id),
             {:ok, bin} <- Static.load(id) do
          Glfw.ToPort.put_texture(port, hash, :file, w, h, bin)
          [id | images]
        else
          _ -> images
        end
      end)

    put_in(state, [:media, :images], images)
  end

  defp ensure_streams(state, []), do: state

  defp ensure_streams(%{port: port, media: media} = state, ids) do
    streams = Map.get(media, :streams, [])

    streams =
      Enum.reduce(ids, streams, fn id, streams ->
        with false <- Enum.member?(streams, id),
             :ok <- Stream.subscribe(id),
             {:ok, {Stream.Image, {w, h, format}, bin}} <- Stream.fetch(id) do
          id32 = gen_id32_from_string(id)
          Glfw.ToPort.put_texture(port, id32, format, w, h, bin)
          [id | streams]
        else
          _ -> streams
        end
      end)

    put_in(state, [:media, :streams], streams)
  end

  # if this is the first time we see this font, we need to send it to the renderer
  defp serialize_font(id) when is_bitstring(id) do
    hash =
      with {:ok, {Static.Font, _}} <- Static.fetch(id),
           {:ok, _bin_hash, str_hash} <- Static.to_hash(id) do
        str_hash
      else
        err -> raise "Invalid font -> #{inspect(id)}, err: #{inspect(err)}"
      end

    [
      <<
        @op_font::16-big,
        byte_size(hash)::16-big
      >>,
      Script.padded_string(hash)
    ]
  end

  defp serialize_fill_stream(id) when is_bitstring(id) do
    [
      <<
        @op_fill_image::16-big,
        0::16-big
      >>,
      gen_id32_from_string(id)
    ]
  end

  defp serialize_stroke_stream(id) when is_bitstring(id) do
    [
      <<
        @op_stroke_image::16-big,
        0::16-big
      >>,
      gen_id32_from_string(id)
    ]
  end

  defp gen_id32_from_string(id) do
    byte_count = byte_size(id)

    case byte_count <= 32 do
      true ->
        bits = (32 - byte_count) * 8
        id <> <<0::size(bits)>>

      false ->
        :crypto.hash(:sha_256, id)
    end
  end
end
