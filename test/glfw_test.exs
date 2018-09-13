defmodule Scenic.Driver.GlfwTest do
  use ExUnit.Case
  doctest Scenic.Driver.Glfw
  alias Scenic.Graph
  alias Scenic.ViewPort
  alias Scenic.Scene
  import Scenic.Primitives
  alias Scenic.Driver.Glfw

  import IEx

  @name   :glfw_test
  @size   {700, 600}
  @config %{
          module: Glfw,
          name: @name,
          opts: [resizeable: false, title: inspect(__MODULE__)],
        }

  @graph_simple Graph.build()
  |> circle(100, stroke: {2, :green}, translate: {200, 200})


  setup do
    %{}
  end

  test "Integration style test" do
    {:ok, state} = Glfw.init(self(), @size, @config)
    assert state.clear_color == {0, 0, 0, 255}
    assert state.closing == false
    assert state.currently_drawing == []
    assert state.debounce == %{}
    assert state.dirty_graphs == []
    assert state.dl_block_size == 512
    assert state.dl_map == %{}
    assert state.draw_busy == false
    assert state.fonts == %{}
    assert state.frame == @size
    assert state.last_used_dl == nil
    assert state.pending_flush == false
    assert is_port(state.port)
    assert state.ready == false
    assert state.root_ref == nil
    assert state.screen_factor == 1.0
    assert state.textures == %{}
    assert state.used_dls == %{}
    assert is_pid(state.viewport)

    # prep
    state = %{state | ready: true, start_dl: 0, end_dl: 512, last_used_dl: 0 }

    # make up a scene_ref and manually push a graph
    scene_ref = make_ref()
    graph_key = {:graph, scene_ref, nil}


    # give the port time to spin up
    Process.sleep(2000)

    # arc
    Graph.build()
    |> arc({100,0,1}, stroke: {2, :green}, translate: {300, 300})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:set_root, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    # circles
    Graph.build()
    |> circle(100, stroke: {2, :green}, translate: {200, 200})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:set_root, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> circle(100, stroke: {8, :green}, fill: :azure, translate: {200, 200})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> circle(100, stroke: {8, :green}, fill: {:radial, {0,0,10,90,:blue,:yellow} }, translate: {200, 200})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> circle(100, stroke: {8, :green}, fill: {:linear, {-100,-100,100,100,:blue,:green} }, translate: {200, 200})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> circle(100, stroke: {8, :green}, fill: {:box, {0,0,100,100,40,10,:red,:yellow} }, translate: {200, 200})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    # ellipse
    Graph.build()
    |> ellipse({100, 150}, stroke: {2, :green}, translate: {200, 200})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:set_root, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> ellipse({100, 150}, stroke: {8, :green}, fill: :azure, translate: {200, 200})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> ellipse({100, 150}, stroke: {8, :green}, fill: {:radial, {0,0,10,90,:blue,:yellow} }, translate: {200, 200})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> ellipse({100, 150}, stroke: {8, :green}, fill: {:linear, {-100,-100,100,100,:blue,:green} }, translate: {200, 200})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> ellipse({100, 150}, stroke: {8, :green}, fill: {:box, {0,0,100,100,40,10,:red,:yellow} }, translate: {200, 200})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)


    # lines
    Graph.build()
    |> line( {{0,0},{200,200}}, stroke: {4, :green} )
    |> line( {{0,200},{200,0}}, stroke: {4, :red} )
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}

    Process.sleep(10000)
  end




  defp test_push_graph( graph, {_, ref, id}) do
    Scene.handle_cast(
      {:push_graph, graph, id, false},
      %{
        scene_ref: ref,
        raw_scene_refs: %{},
        dyn_scene_pids: %{},
        dyn_scene_keys: %{},
        dynamic_children_pid: self(),
        viewport: self()
      }
    )
  end

end
