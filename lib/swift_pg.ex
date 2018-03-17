defmodule SwiftPg do
  @moduledoc """
  Documentation for SwiftPg.
  """

  @on_load :init

  defp init do
    :erlang.load_nif "priv/swift_pg", 0
  end

  def connect, do: connect(%{})
  def connect(_), do: {:error, "nif not loaded"}

  def query(r, sql), do: query(r, sql, [])
  def query(_r, _sql, _list), do: {:error, "nif not loaded"}
end
