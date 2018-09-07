defmodule Scenic.Driver.Glfw.MixProject do
  use Mix.Project

  def project do
    [
      app: :scenic_driver_glfw,
      version: "0.7.0",
      build_path: "_build",
      config_path: "config/config.exs",
      deps_path: "deps",
      lockfile: "mix.lock",
      elixir: "~> 1.6",
      description: description(),
      build_embedded: Mix.env() == :prod,
      start_permanent: Mix.env() == :prod,
      compilers: [:elixir_make] ++ Mix.compilers(),
      make_env: %{"MIX_ENV" => to_string(Mix.env())},
      make_clean: ["clean"],
      deps: deps(),
      dialyzer: [plt_add_deps: :transitive]
    ]
  end

  # Run "mix help compile.app" to learn about applications.
  def application do
    [
      extra_applications: [:logger]
    ]
  end

  # Run "mix help deps" to learn about dependencies.
  defp deps do
    [
      {:elixir_make, "~> 0.4"},
      {:scenic, git: "git@github.com:boydm/scenic.git"},
      {:dialyxir, "~> 0.5", only: :dev, runtime: false}
    ]
  end

  defp description() do
    """
    Scenic.Driver.Glfw - Main Scenic driver for MacOs and Ubuntu
    """
  end
end
