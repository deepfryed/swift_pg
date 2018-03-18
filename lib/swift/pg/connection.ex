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
    case do_exec(c, sql, typecast(list)) do
      {:ok, r} -> {:ok, Swift.Pg.Result.new(r)}
      error    -> error
    end
  end

  defp init do
    Swift.Pg.Result.init
    :erlang.load_nif "priv/swift/pg/connection", 0
  end

  defp do_exec(_c, _sql, _list), do: {:error, "nif not loaded"}

  defp typecast([]), do: []
  defp typecast([_ | _] = list) do
    Enum.map(list, &typecast_value(&1))
  end

  defp typecast_value(v) when v in [nil, true, false], do: v
  defp typecast_value(%{} = v) do
    Poison.encode!(v)
  end
  defp typecast_value([_ | _] = list) do
    s =
      list
      |> Enum.map(& array_el &1)
      |> Enum.join(",")
    "{#{s}}"
  end
  defp typecast_value(v), do: to_string(v)

  defp array_el(v) when is_number(v), do: v
  defp array_el(v) when is_nil(v), do: "NULL"
  defp array_el(v) when is_boolean(v), do: to_string(v)
  defp array_el(v) when is_map(v), do: "'#{Poison.encode!(v)}'"
  defp array_el(v) when is_list(v), do: typecast_value(v)
  defp array_el(v), do: "'#{to_string(v)}'"
end
