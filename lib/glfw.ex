#
#  Created by Boyd Multerer on 05/31/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#
#  sends data to a Glfw port app
#
defmodule Scenic.Driver.Glfw do
  @moduledoc """

  """

  use Scenic.ViewPort.Driver
  alias Scenic.Cache

  alias Scenic.Driver.Glfw

  # import IEx

  require Logger

  @port '/' ++ to_charlist(Mix.env()) ++ '/scenic_driver_glfw'

  @default_title "Driver Glfw"
  @default_resizeable false

  @default_block_size 512

  @default_clear_color {0, 0, 0, 0xFF}

  @default_sync 15

  # ============================================================================
  # client callable api

  def query_stats(pid), do: GenServer.call(pid, :query_stats)
  def reshape(pid, width, height), do: GenServer.cast(pid, {:reshape, width, height})
  def position(pid, x, y), do: GenServer.cast(pid, {:position, x, y})
  def focus(pid), do: GenServer.cast(pid, :focus)
  def iconify(pid), do: GenServer.cast(pid, :iconify)
  def maximize(pid), do: GenServer.cast(pid, :maximize)
  def restore(pid), do: GenServer.cast(pid, :restore)
  def show(pid), do: GenServer.cast(pid, :show)
  def hide(pid), do: GenServer.cast(pid, :hide)
  def close(pid), do: GenServer.cast(pid, :close)

  if Mix.env() == :dev do
    def crash(pid), do: GenServer.cast(pid, :crash)
  end

  # ============================================================================
  # startup

  def init(viewport, {width, height}, config) do
    title =
      cond do
        is_bitstring(config[:title]) -> config[:title]
        true -> @default_title
      end

    resizeable =
      cond do
        is_boolean(config[:resizeable]) -> config[:resizeable]
        true -> @default_resizeable
      end

    sync_interval =
      cond do
        is_integer(config[:sync]) -> config[:sync]
        true -> @default_sync
      end

    dl_block_size =
      cond do
        is_integer(config[:block_size]) -> config[:block_size]
        true -> @default_block_size
      end

    port_args =
      to_charlist(" #{width} #{height} #{inspect(title)} #{resizeable} #{dl_block_size}")

    # request put and delete notifications from the cache
    Cache.Static.Font.subscribe(:all)
    Cache.Static.Texture.subscribe(:all)
    
    # open and initialize the window
    Process.flag(:trap_exit, true)
    executable = :code.priv_dir(:scenic_driver_glfw) ++ @port ++ port_args
    port = Port.open({:spawn, executable}, [:binary, {:packet, 4}])

    state = %{
      inputs: 0x0000,
      port: port,
      closing: false,
      ready: false,
      debounce: %{},
      root_ref: nil,
      dl_block_size: dl_block_size,
      start_dl: nil,
      end_dl: nil,
      last_used_dl: nil,
      dl_map: %{},
      used_dls: %{},
      clear_color: @default_clear_color,
      textures: %{},
      fonts: %{},
      dirty_graphs: [],
      sync_interval: sync_interval,
      draw_busy: false,
      pending_flush: false,
      currently_drawing: [],
      window: {width, height},
      frame: {width, height},
      screen_factor: 1.0,
      viewport: viewport
    }

    # mark this rendering process has high priority
    # Process.flag(:priority, :high)

    {:ok, state}
  end

  # ============================================================================
  # farm out handle_cast and handle_info to the supporting modules.
  # this module just got too long and complicated, so this cleans things up.

  # --------------------------------------------------------
  def handle_call(msg, from, state) do
    Glfw.Port.handle_call(msg, from, state)
  end

  # --------------------------------------------------------
  # %{ready: true} =
  def handle_cast(msg, state) do
    msg
    |> do_handle(&Glfw.Graph.handle_cast(&1, state))
    |> do_handle(&Glfw.Cache.handle_cast(&1, state))
    |> do_handle(&Glfw.Port.handle_cast(&1, state))
    # |> do_handle( &Glfw.Font.handle_cast( &1, state ) )
    |> case do
      {:noreply, state} ->
        {:noreply, state}

      _ ->
        {:noreply, state}
    end
  end

  #  def handle_cast( {:request_input, input_flags}, state ) do
  #    {:noreply, Map.put(state, :inputs, input_flags)}
  #  end
  #  def handle_cast( {:cache_put, _}, state ), do: {:noreply, state}
  #  def handle_cast( {:cache_delete, _}, state ), do: {:noreply, state}
  #  def handle_cast( msg, %{ready: false} = state ) do
  #    {:noreply, state}
  #  end

  # --------------------------------------------------------
  def handle_info(:flush_dirty, %{ready: true} = state) do
    Glfw.Graph.handle_flush_dirty(state)
  end

  # --------------------------------------------------------
  def handle_info({:debounce, type}, %{ready: true} = state) do
    Glfw.Input.handle_debounce(type, state)
  end

  # --------------------------------------------------------
  def handle_info({msg_port, {:data, msg}}, %{port: port} = state) when msg_port == port do
    msg
    |> do_handle(&Glfw.Input.handle_port_message(&1, state))
  end

  # deal with the app exiting normally
  def handle_info({:EXIT, port_id, :normal} = msg, %{port: port, closing: closing} = state)
      when port_id == port do
    if closing do
      Logger.info("clean close")
      # we are closing cleanly, let it happen.
      GenServer.stop(self())
      {:noreply, state}
    else
      Logger.error("dirty close")
      # we are not closing cleanly. Let the supervisor recover.
      super(msg, state)
    end
  end

  def handle_info(msg, state) do
    super(msg, state)
  end

  # --------------------------------------------------------
  defp do_handle({:noreply, _} = msg, _), do: msg

  defp do_handle(msg, handler) when is_function(handler) do
    handler.(msg)
  end
end
