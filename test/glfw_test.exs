defmodule Scenic.Driver.GlfwTest do
  use ExUnit.Case
  doctest Scenic.Driver.Glfw
  alias Scenic.Graph
  alias Scenic.Scene
  alias Scenic.Cache
  import Scenic.Primitives
  alias Scenic.Driver.Glfw

  # import IEx

  @name   :glfw_test
  @size   {700, 600}
  @config %{
          module: Glfw,
          name: @name,
          opts: [resizeable: false, title: inspect(__MODULE__)],
        }


  @triangle {{0, 260}, {250, 0}, {250, 260}}

  @font_hash "o3FsNZ8jSxxunWBCLXtpVhTd06Q"
  @font_path "test/static/Indie_Flower/IndieFlower.ttf"

  @parrot_hash "UfHCVlANI2cFbwSpJey64FxjT-0"
  @parrot_path "test/static/scenic_parrot.png"

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

    # load the parrot texture into the cache
    Scenic.Cache.File.load(@parrot_path, @parrot_hash)

    # give the port time to spin up
    Process.sleep(1500)
    Glfw.Cache.load_texture(@parrot_hash, state.port)

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

    Graph.build()
    |> circle(100, stroke: {8, :green}, fill: {:image, @parrot_hash }, translate: {200, 200})
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
    Process.sleep(40)

    # path
    Graph.build()
    |> path( [
        :begin,
        {:move_to, 0, 0},
        {:bezier_to, 0, 20, 0, 50, 40, 50},
        {:bezier_to, 60, 50, 60, 20, 80, 20},
        {:bezier_to, 100, 20, 110, 0, 120, 0},
        {:bezier_to, 140, 0, 160, 30, 160, 50}
      ], stroke: {4, :green} )
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}

    # quad
    Graph.build()
    |> quad( {{0,20},{30,0},{36,26},{25,40}}, stroke: {4, :green}, fill: :purple )
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    # rect
    Graph.build()
    |> rect( {200,100}, stroke: {4, :green}, fill: :red, translate: {40, 60} )
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    # rounded rect
    Graph.build()
    |> rrect( {200,100, 20}, stroke: {4, :green}, fill: :red, translate: {40, 60} )
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    # sector
    Graph.build()
    |> sector({200,0,1}, stroke: {2, :yellow}, translate: {100, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> sector({200,0,1}, stroke: {8, {:radial, {0,0,10,90,:blue,:yellow} }}, translate: {100, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> sector({200,0,1}, stroke: {8, {:linear, {-100,-100,100,100,:blue,:green} }}, translate: {100, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> sector({200,0,1}, stroke: {8, {:box, {0,0,100,100,40,10,:red,:yellow} }}, translate: {100, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    # text
    Glfw.Font.load_font(:roboto, state.port)
    Glfw.Font.load_font(:roboto_mono, state.port)
    Glfw.Font.load_font(:roboto_slab, state.port)
    
    Graph.build(font: :roboto, font_size: 24)
    |> text("This is some text", fill: :yellow, translate: {200, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)
    
    Graph.build(font: :roboto_mono, font_size: 30)
    |> text("This is some text", fill: :yellow, translate: {200, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)
    
    Graph.build(font: :roboto_slab, font_size: 40)
    |> text("This is some text", fill: :yellow, translate: {200, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)
    
    Graph.build(font: :roboto, font_size: 24, text_align: :right)
    |> text("This is some text", fill: :yellow, translate: {200, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)
    
    Graph.build(font: :roboto_mono, font_size: 30, text_align: :center)
    |> text("This is some text", fill: :yellow, translate: {200, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)
    
    Graph.build(font: :roboto_slab, font_size: 40, font_blur: 2)
    |> text("This is some text, blurred", fill: :yellow, translate: {200, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)
    
    # custom font
    Graph.build(font: :roboto_slab, font_size: 40, font_blur: 2)
    |> text("This is some text, blurred", fill: :yellow, translate: {200, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)


    # custom font
    assert Cache.File.load(@font_path, @font_hash) == {:ok, @font_hash}
    Glfw.Font.load_font(@font_hash, state.port)
    Graph.build(font: @font_hash, font_size: 60)
    |> text("From a cached font", fill: :azure, translate: {100, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)
    Glfw.Font.free_font(@font_hash, state.port)


    # triangles
    Graph.build()
    |> triangle(@triangle, stroke: {2, :green}, translate: {100, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> triangle(@triangle, stroke: {8, :green}, fill: :azure, translate: {100, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> triangle(@triangle, stroke: {8, :green}, fill: {:radial, {0,0,10,90,:blue,:yellow} }, translate: {100, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> triangle(@triangle, stroke: {8, :green}, fill: {:box, {0,0,100,100,40,10,:red,:yellow} }, translate: {100, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    state =  %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)

    Graph.build()
    |> triangle(@triangle, stroke: {8, :yellow}, fill: {:linear, {0,0,200,200,:blue,:green} }, translate: {100, 100})
    |> test_push_graph(graph_key)
    {:noreply, state} = Glfw.handle_cast( {:update_graph, graph_key}, state )
    %{state | pending_flush: false, dirty_graphs: []}
    Process.sleep(40)
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
