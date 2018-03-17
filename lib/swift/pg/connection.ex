defmodule Swift.Pg.Connection do
  @moduledoc """
  Postgres connection.
  """

  @on_load :init

  defp init do
    :erlang.load_nif "priv/swift/pg/connection", 0
  end

  def new, do: new(%{})
  def new(_), do: {:error, "nif not loaded"}

  def exec(r, sql), do: exec(r, sql, [])
  def exec(_r, _sql, _list), do: {:error, "nif not loaded"}
end
