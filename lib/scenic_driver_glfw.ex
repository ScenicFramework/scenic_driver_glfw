defmodule Scenic.Driver.Glfw do
  @opts_schema [
    name: [type: {:or, [:atom, :string]}],
    title: [type: :string, default: "Driver Glfw"],
    limit_ms: [type: :non_neg_integer, default: 29],
    resizeable: [type: :boolean, default: false],
    on_close: [
      type: {:or, [:mfa, {:in, [:stop_driver, :stop_viewport, :stop_system, :halt_system]}]},
      default: :stop_system
    ]
  ]

  @moduledoc """
  Supported config options:\n#{NimbleOptions.docs(@opts_schema)}
  """

  use Scenic.Driver
  require Logger

  alias Scenic.Script
  alias Scenic.ViewPort
  alias Scenic.Driver.Glfw.ToPort
  alias Scenic.Driver.Glfw.FromPort
  alias Scenic.Assets.Static
  alias Scenic.Assets.Stream

  # import IEx

  @driver_ext if elem(:os.type(), 0) == :win32, do: '.exe', else: ''
  @port '/' ++ to_charlist(Mix.env()) ++ '/scenic_driver_glfw' ++ @driver_ext

  # same as scenic/script.ex
  @op_draw_script 0x0F
  @op_font 0x90
  @op_fill_image 0x63
  @op_stroke_image 0x74

  @root_id ViewPort.root_id()

  @impl Scenic.Driver
  def validate_opts(opts), do: NimbleOptions.validate(opts, @opts_schema)

  # --------------------------------------------------------
  @doc false
  @impl Scenic.Driver
  def init(driver, opts) do
    {width, height} = driver.viewport.size
    # IO.puts("================> INIT GLFW DRIVER #{inspect(driver)} <================")

    # prepare the agr string to start the port exe
    port_args = " #{width} #{height} #{opts[:resizeable]} \"#{opts[:title]}\""

    # open and initialize the window
    Process.flag(:trap_exit, true)
    executable = :code.priv_dir(:scenic_driver_glfw) ++ @port ++ to_charlist(port_args)
    port = Port.open({:spawn, executable}, [:binary, {:packet, 4}])

    driver =
      assign(driver,
        port: port,
        screen_factor: 1.0,
        size: {width, height},
        closing: false,
        on_close: opts[:on_close],
        media: %{},
        script_ids: %{@root_id => 0},
        next_script_id: 1,
        opts: opts,
        busy: true
      )

    {:ok, driver}
  end

  # --------------------------------------------------------
  @doc false
  @impl Scenic.Driver
  def request_input(input, %{assigns: %{port: port}} = driver) do
    ToPort.request_inputs(input, port)
    {:ok, driver}
  end

  # --------------------------------------------------------
  @doc false
  @impl Scenic.Driver
  def reset_scene(%{assigns: %{port: port}} = driver) do
    Stream.unsubscribe(:all)
    ToPort.reset_start(port)

    # state changes
    driver =
      assign(driver,
        script_ids: %{@root_id => 0},
        next_script_id: 1,
        busy: false,
        media: %{}
      )

    {:ok, driver}
  end

  # --------------------------------------------------------
  @doc false
  @impl Scenic.Driver
  def put_scripts(ids, %{assigns: %{port: port}} = driver) do
    driver = do_put_scripts(driver, ids)
    ToPort.render(port)
    {:ok, driver}
  end

  # --------------------------------------------------------
  @doc false
  @impl Scenic.Driver
  def del_scripts(ids, %{assigns: %{port: port}} = driver) do
    Enum.each(ids, &ToPort.del_script(&1, port))
    {:ok, driver}
  end

  # --------------------------------------------------------
  # deal with the app exiting normally
  @impl GenServer
  def handle_info({:EXIT, port_id, :normal}, %{assigns: %{port: port, closing: closing}} = driver)
      when port_id == port do
    if closing do
      Logger.info("clean close")
      # we are closing cleanly, let it happen.
      GenServer.stop(self())
      {:noreply, driver}
    else
      Logger.error("dirty close")
      # we are not closing cleanly. Let the supervisor recover.
      {:noreply, driver}
    end
  end

  # --------------------------------------------------------
  # streaming asset updates
  def handle_info({{Stream, :put}, Stream.Image, id}, %{assigns: %{port: port}} = driver) do
    with {:ok, {Stream.Image, {w, h, _mime}, bin}} <- Stream.fetch(id) do
      id32 = gen_id32_from_string(id)
      ToPort.put_texture(port, id32, :file, w, h, bin)
    end

    {:noreply, driver}
  end

  def handle_info({{Stream, :put}, Stream.Bitmap, id}, %{assigns: %{port: port}} = driver) do
    with {:ok, {Stream.Bitmap, {w, h, type}, bin}} <- Stream.fetch(id) do
      id32 = gen_id32_from_string(id)
      ToPort.put_texture(port, id32, type, w, h, bin)
    end

    {:noreply, driver}
  end

  def handle_info({{Stream, _verb}, _type, _id}, driver), do: {:noreply, driver}

  # --------------------------------------------------------
  # messages from the port
  def handle_info({pid, {:data, data}}, %{assigns: %{port: port}} = driver) when pid == port do
    FromPort.handle_port_message(data, driver)
  end

  def handle_info(msg, driver) do
    IO.inspect(msg, label: "GLFW UNHANDLED INFO")
    {:noreply, driver}
  end

  # --------------------------------------------------------
  defp do_put_scripts(%{assigns: %{port: port}, viewport: vp} = state, ids) do
    Enum.reduce(ids, state, fn id, state ->
      with {:ok, script} <- ViewPort.get_script(vp, id) do
        state = ensure_media(script, state)
        {s_id, state} = ensure_script_id(id, state)

        {io, state} =
          Script.serialize(script, state, fn
            {:script, id}, state -> serialize_script(id, state)
            {:font, id}, state -> {serialize_font(id), state}
            {:fill_stream, id}, state -> {serialize_fill_stream(id), state}
            {:stroke_stream, id}, state -> {serialize_stroke_stream(id), state}
            other, state -> {other, state}
          end)

        ToPort.put_script(io, s_id, port)

        state
      else
        _ -> state
      end
    end)
  end

  defp ensure_media(script, state) do
    media = Script.media(script)

    state
    |> ensure_fonts(Map.get(media, :fonts, []))
    |> ensure_images(Map.get(media, :images, []))
    |> ensure_streams(Map.get(media, :streams, []))
  end

  defp ensure_fonts(state, []), do: state

  defp ensure_fonts(%{assigns: %{port: port, media: media}} = driver, ids) do
    fonts = Map.get(media, :fonts, [])

    fonts =
      Enum.reduce(ids, fonts, fn id, fonts ->
        with false <- Enum.member?(fonts, id),
             {:ok, {Static.Font, _}} <- Static.meta(id),
             {:ok, str_hash} <- Static.to_hash(id),
             {:ok, bin} <- Static.load(id) do
          ToPort.put_font(port, str_hash, bin)
          [id | fonts]
        else
          _ -> fonts
        end
      end)

    assign(driver, :media, Map.put(media, :fonts, fonts))
  end

  defp ensure_images(state, []), do: state

  defp ensure_images(%{assigns: %{port: port, media: media}} = state, ids) do
    images = Map.get(media, :images, [])

    images =
      Enum.reduce(ids, images, fn id, images ->
        with false <- Enum.member?(images, id),
             {:ok, {Static.Image, {w, h, _}}} <- Static.meta(id),
             {:ok, str_hash} <- Static.to_hash(id),
             {:ok, bin_hash} <- Base.url_decode64(str_hash, padding: false),
             {:ok, bin} <- Static.load(id) do
          ToPort.put_texture(port, bin_hash, :file, w, h, bin)
          [id | images]
        else
          _ -> images
        end
      end)

    assign(state, :media, Map.put(media, :images, images))
  end

  defp ensure_streams(state, []), do: state

  defp ensure_streams(%{assigns: %{port: port, media: media}} = state, ids) do
    streams = Map.get(media, :streams, [])

    streams =
      Enum.reduce(ids, streams, fn id, streams ->
        with false <- Enum.member?(streams, id),
             :ok <- Stream.subscribe(id) do
          case Stream.fetch(id) do
            {:ok, {Stream.Image, {w, h, _format}, bin}} ->
              id32 = gen_id32_from_string(id)
              ToPort.put_texture(port, id32, :file, w, h, bin)
              [id | streams]

            {:ok, {Stream.Bitmap, {w, h, format}, bin}} ->
              id32 = gen_id32_from_string(id)
              ToPort.put_texture(port, id32, format, w, h, bin)
              [id | streams]

            _err ->
              streams
          end
        else
          _ -> streams
        end
      end)

    assign(state, :media, Map.put(media, :streams, streams))
  end

  # if this is the first time we see this font, we need to send it to the renderer
  defp serialize_font(id) when is_bitstring(id) do
    hash =
      with {:ok, {Static.Font, _}} <- Static.meta(id),
           {:ok, str_hash} <- Static.to_hash(id) do
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

  defp ensure_script_id(
         script_id,
         %{
           assigns: %{
             script_ids: script_ids,
             next_script_id: next_script_id
           }
         } = driver
       )
       when is_bitstring(script_id) do
    case Map.fetch(script_ids, script_id) do
      {:ok, id} ->
        {id, driver}

      :error ->
        driver =
          assign(driver,
            script_ids: Map.put(script_ids, script_id, next_script_id),
            next_script_id: next_script_id + 1
          )

        {next_script_id, driver}
    end
  end

  defp serialize_script(script_id, state) when is_bitstring(script_id) do
    {s_id, state} = ensure_script_id(script_id, state)
    {<<@op_draw_script::16-big, s_id::16>>, state}
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
