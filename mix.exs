defmodule Scenic.Driver.Glfw.MixProject do
  use Mix.Project

  @github "https://github.com/boydm/scenic_driver_glfw"
  @version "0.9.1"

  def project do
    [
      app: :scenic_driver_glfw,
      version: @version,
      build_path: "_build",
      config_path: "config/config.exs",
      deps_path: "deps",
      lockfile: "mix.lock",
      elixir: "~> 1.6",
      description: description(),
      build_embedded: true,
      start_permanent: Mix.env() == :prod,
      compilers: [:elixir_make] ++ Mix.compilers(),
      make_env: %{"MIX_ENV" => to_string(Mix.env())},
      make_clean: ["clean"],
      deps: deps(),
      dialyzer: [plt_add_deps: :transitive],
      package: [
        name: :scenic_driver_glfw,
        contributors: ["Boyd Multerer"],
        maintainers: ["Boyd Multerer"],
        licenses: ["Apache 2"],
        links: %{github: @github},
        files: [
          "c_src/**/*.[ch]",
          "c_src/**/*.txt",
          "config",
          "priv/fonts",
          "lib/**/*.ex",
          "Makefile",
          "mix.exs",
          "README.md",
          "LICENSE",
          "changelist.md"
        ]
      ],
      docs: [
        extras: doc_guides(),
        main: "overview",
        source_ref: "v#{@version}",
        source_url: "https://github.com/boydm/scenic_driver_glfw"
        # homepage_url: "http://kry10.com",
      ]
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
      # {:scenic, "~> 0.10"},
      {:scenic, git: "https://github.com/boydm/scenic.git"},
      {:ex_doc, ">= 0.0.0", only: [:dev]},
      {:dialyxir, "~> 0.5", only: :dev, runtime: false}
    ]
  end

  defp description() do
    """
    Scenic.Driver.Glfw - Main Scenic driver for MacOs and Ubuntu
    """
  end

  defp doc_guides do
    [
      "guides/overview.md"
    ]
  end
end
