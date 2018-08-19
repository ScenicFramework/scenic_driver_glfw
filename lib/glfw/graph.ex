#
#  Created by Boyd Multerer on 02/15/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#
# a collection of functions for maintaining and drawing graphs. These could have
# just as well been in Scenic.Driver.Glfw itself, but that was getting too long
# and complicated
#
defmodule Scenic.Driver.Glfw.Graph do
  alias Scenic.Driver.Glfw
  alias Scenic.Driver.Glfw.Port
  alias Scenic.ViewPort
  alias Scenic.Primitive
  alias Scenic.Utilities

  require Logger

  @cmd_render_graph         0x01
  @msg_draw_ready_id        0x07

  import IEx

  #--------------------------------------------------------
  # render graphs in the dirty_graphs list. They were place there
  # by being rendered too quickly after the previous render cycle.
  # Enum.uniq means that if a graph is update multiple times within
  # a single frame, it will only be rendered out once
  def handle_flush_dirty( %{
    draw_busy: true,
    sync_interval: sync_interval
  } = state ) do
    # already busy drawing. Try again later.
    # This is similar to skipping a frame
    Process.send_after(self(), :flush_dirty, sync_interval)
    {:noreply, state }
  end

  def handle_flush_dirty( %{
    dirty_graphs: dg
  } = state ) do

    state = dg
    |> Enum.uniq()
    |> Enum.reverse()
    |> render_graphs( state )
    # returned parameter is now state...
    |> Map.put(:dirty_graphs, [])
    |> Map.put(:pending_flush, false)
    
    {:noreply, state }
  end



  #============================================================================
  # main incoming graph messages

  @doc false
  def handle_cast( msg, state)


  #--------------------------------------------------------
  def handle_cast( :update_clear_color,
    %{port: port, root_ref: root_key, clear_color: old_clear_color} = state
  ) do
    state = with {:ok, graph} <- ViewPort.Tables.get_graph( root_key ) do
      root_group = graph[0]
      clear_color = ((root_group
          |> Map.get(:styles, %{})
          |> Map.get(:clear_color)) ||
        ( root_group
          |> Map.get(:styles, %{})
          |> Map.get(:theme, %{})
          |> Map.get(:background, :black)))
      |> Primitive.Style.Paint.Color.normalize()

      if clear_color != old_clear_color do
        # the color has changed. deal with it
        Port.clear_color(port, clear_color)
        {:noreply, %{state | clear_color: clear_color}}
      else
        # hasn't changed. Don't do anything
        {:noreply, state }
      end
    else
      _ ->
        # the graph isn't in the table. don't do anything
        {:noreply, state }
    end
  end

  #--------------------------------------------------------
  def handle_cast( {:request_input, _input_flags}, %{port: _port} = state) do
#    send_input_flags( input_flags, port )
    {:noreply, state}
  end

  #--------------------------------------------------------
  def handle_cast( {:set_root, graph_key}, %{
    ready: true,
    port: port
  } = state ) do
    # Logger.warn "Glfw set_root #{inspect(graph_key)}"


    state = Map.put(state, :root_ref, graph_key)

    # build a list of keys to render
    keys = recursive_graph_keys( graph_key )
    |> Enum.uniq()
    |> Enum.reverse()

    # render immediatly - sets graph_key as the root
    state = render_graphs( keys, state, graph_key )

    # update the clear color
    GenServer.cast(self(), :update_clear_color)

    # {:noreply, %{state | root_ref: graph_key}}
    {:noreply, state}
  end

  #--------------------------------------------------------
  def handle_cast( {:update_graph, graph_key}, %{
    ready: true,
    pending_flush: false,
    sync_interval: sync_interval
  } = state ) do
    # render the graph immediately to reduce latencey
    state = render_graphs( graph_key, state )

    # queue up a :flush_dirty message to catch future fast-follow renders
    Process.send_after(self(), :flush_dirty, sync_interval)
    {:noreply, %{state | pending_flush: true}}
  end

  #--------------------------------------------------------
  def handle_cast( {:update_graph, graph_key}, %{
    ready: true,
    pending_flush: true,
    dirty_graphs: dg
  } = state ) do
    # there is a pending :flush_dirty messsage
    # simply add the key to the dirty_graphs list
    {:noreply, %{state | dirty_graphs: [graph_key | dg]}}
  end

  #--------------------------------------------------------
  def handle_cast( {:delete_graph, graph_key}, %{
    port: port,
    dl_map: dl_map,
    dirty_graphs: dg
  } = state ) do
    state = case dl_map[graph_key] do
      nil ->
        state
      dl_id ->
        # clear the dl
        Port.clear_dl(port, dl_id)
        # free the dl to go back into the pool
        state
        |> Utilities.Map.delete_in( [:dl_map, graph_key] )
        |> Utilities.Map.delete_in( [:used_dls, dl_id] )
    end
    ViewPort.Tables.unsubscribe( graph_key, self() )

    # make sure the key is not marked as dirty
    dg = Enum.reject(dg, fn(k) -> k == graph_key end)

    {:noreply, %{state | dirty_graphs: dg}}
  end


  #--------------------------------------------------------
  # unhandled. do nothing
  def handle_cast( msg, _state ) do
    msg
  end


  #============================================================================
  # render utilities

  #--------------------------------------------------------
  # recursively build a list of all graphs keys
  defp recursive_graph_keys( graph_key, keys \\ [] ) do
    keys = with {:ok, references} <- ViewPort.Tables.get_refs(graph_key) do
      Enum.reduce(references, keys, fn({_, key}, ks) ->
        recursive_graph_keys( key, ks )
      end)
    else
      _ ->
        keys
    end

    # return the id in the list of ids
    [graph_key | keys]
    |> Enum.uniq()
  end


  #--------------------------------------------------------
  defp render_graphs( keys, state, root_key \\ nil )
  defp render_graphs( {:graph,_,_} = key, state, root_key ) do
    render_graphs( [key], state, root_key )
  end
  defp render_graphs( [], state, _ ), do: state
  defp render_graphs( keys, %{
    port: port
  } = state, root_key ) when is_list(keys) do
    keys = Enum.uniq(keys)

    # first, scan the graphs and ensure that dl_ids are properly assigned
    {state, ids} = Enum.reduce( keys, {state, []}, fn(key, {s, ids}) ->
      s = ensure_ids(key, s)
      ids = [ get_dl_id(key, s) | ids ]
      {s, ids}
    end)

    # in a seperate task, render the graphs
    # this is done in a task to both allow for easy memory cleanup
    # and to continue processing messages in the main process while
    # the C code is busy doing it's work
    driver = self()
    root_id = case root_key do
      nil -> nil
      key -> get_dl_id(key, state)
    end
    Task.start_link fn ->
      Enum.each( keys, &render_one_graph(driver, &1, state) )
      if root_id, do: Port.set_root_dl(port, root_id)
    end

# IO.puts "RENDER #{inspect(ids, charlists: :as_lists)}"
    %{state | currently_drawing: ids, draw_busy: true}
  end

  #--------------------------------------------------------
  # register the graph and its refswith the C driver. return the updated dl_map
  # do nothing if the graph is already registered.
  defp ensure_ids( graph_key, state ) do
    state = ensure_one_id( graph_key, state )
    # make sure the graph is real and get its refs at the same time
    with {:ok, references} <- ViewPort.Tables.get_refs( graph_key ) do
      # the references need to be there too
      Enum.reduce(references, state, fn({_, key}, s) ->
        ensure_ids( key, s )
      end)
    else
      _ -> state
    end
  end

  defp ensure_one_id( graph_key, state ) do
    case get_dl_id( graph_key, state ) do
      nil ->
        # this graph is not registered. make it so.
        {:ok, dl_id} = find_open_dl_id( state )
        subscribe_to_graph( self(), graph_key )
        state = state
        |> put_in( [:dl_map, graph_key], dl_id)
        |> put_in( [:used_dls, dl_id], graph_key)
        |> Map.put( :last_used_dl, dl_id )
        render_graphs([graph_key], state)

      _ ->
        # the id is already registered
        state
    end
  end

  defp find_open_dl_id( %{last_used_dl: last_used_dl} = state ) do
    do_find_open_dl_id( last_used_dl + 1, last_used_dl, state )
  end

  defp do_find_open_dl_id( try_id, stop_id, _ ) when try_id == stop_id do
    {:error, :no_dl_id_free}
  end

  defp do_find_open_dl_id( try_id, stop_id, %{
    start_dl:   start_dl,
    end_dl:     end_dl
  } = state ) when try_id > end_dl do
    do_find_open_dl_id( start_dl, stop_id, state )
  end

  defp do_find_open_dl_id( try_id, stop_id, %{used_dls: used_dls} = state ) do
    case used_dls[try_id] do
      nil ->
        # found one!
        {:ok, try_id}
      _ ->
        # keep looking
        do_find_open_dl_id( try_id + 1, stop_id, state )
    end
  end

  #--------------------------------------------------------
  defp get_dl_id( {:graph,_,_} = key, %{dl_map: dl_map}) do
    dl_map[key]
  end

  #--------------------------------------------------------
  # render_one_graph renders one graph out to the C port.
  # It uses data in state but doesn't transform it
  defp render_one_graph( driver, graph_key, %{
    port: port,
    dl_map: dl_map,
    root_ref: root_ref,
    clear_color: old_clear_color
  } = state ) do
    dl_id = dl_map[graph_key]

    with {:ok, graph} <- ViewPort.Tables.get_graph( graph_key ) do

      # if this is the root, check if it has a clear_color set on it.
      if graph_key == root_ref do
        GenServer.cast(driver, :update_clear_color)
      end

      # hack the driver into the state map
      state = Map.put(state, :driver, driver)
      [
        <<
          @cmd_render_graph :: unsigned-integer-size(32)-native,
          dl_id :: unsigned-integer-size(32)-native
        >>,
        Glfw.Compile.graph( graph, graph_key, state )
      ]
      |> Port.send( port )
    else
      _ ->
        # the C driver didn't get called.
        # Since the id is still in the currently_drawing list we
        # need to send a signal that it "drew" or else this genserver
        # will be waiting for the signal from the driver forever.
        # we can't just remove it directly here as this is most
        # likely running in a temporary process.
        # Send a fake completion event as if it came from the port.
        msg = <<
          @msg_draw_ready_id :: unsigned-integer-size(32)-native,
          dl_id :: unsigned-integer-size(32)-native
        >>
        Process.send(driver, {port, {:data, msg}}, [])
    end
  end


  defp subscribe_to_graph( pid, graph_key ) do
    ViewPort.Tables.subscribe( graph_key, pid )
    # listen to the scenes the root refers to
    with {:ok, ref_keys} <- ViewPort.Tables.get_refs(graph_key) do
      Enum.each(ref_keys, fn({_,key}) ->
        subscribe_to_graph(pid, key)
      end)
    end
  end

end















