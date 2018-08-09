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

      build_embedded: Mix.env == :prod,
      start_permanent: Mix.env == :prod,
      compilers: [:elixir_make] ++ Mix.compilers,
      make_env: %{"MIX_ENV" => to_string(Mix.env)},
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
      # {:dep_from_hexpm, "~> 0.3.0"},
      # {:dep_from_git, git: "https://github.com/elixir-lang/my_dep.git", tag: "0.1.0"},
      # {:sibling_app_in_umbrella, in_umbrella: true},

      { :elixir_make, "~> 0.4" },

      { :scenic, git: "git@github.com:boydm/scenic.git" },
      # { :scenic_math, git: "git@github.com:boydm/scenic_math.git" },
      # { :scenic_truetype, git: "git@github.com:boydm/scenic_truetype.git" },
      {:dialyxir, "~> 0.5", only: :dev, runtime: false}
    ]
  end
end
