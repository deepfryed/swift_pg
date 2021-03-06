defmodule SwiftPg.Mixfile do
  use Mix.Project

  def project do
    [
      app: :swift_pg,
      version: "0.1.0",
      elixir: "~> 1.5",
      start_permanent: Mix.env == :prod,
      deps: deps()
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
      {:binaries, path: "./priv", compile: "make -s", app: false},
      {:poison, "~> 2.2 or ~> 3.0"},
    ]
  end
end
