defmodule Swift.Pg.Connection do
  @moduledoc """
  Postgres connection.
  """

  require Swift.Pg.Result

  @on_load :init

  def new, do: new(%{})
  def new(_), do: {:error, "nif not loaded"}

  def exec(c, sql), do: exec(c, sql, [])
  def exec(c, sql, list) do
    case do_exec(c, sql, list) do
      {:ok, r} -> {:ok, Swift.Pg.Result.new(r)}
      error    -> error
    end
  end

  defp init do
    :erlang.load_nif "priv/swift/pg/connection", 0
  end

  defp do_exec(_c, _sql, _list), do: {:error, "nif not loaded"}
end
