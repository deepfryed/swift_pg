defmodule Swift.Pg.Result do
  defstruct result: nil, count: 0, cursor: 0

  @on_load :init

  def new(result) do
    %Swift.Pg.Result{result: result, count: ntuples(result)}
  end

  def ntuples(nil), do: "invalid result set"
  def ntuples(_r), do: raise "nif not loaded"

  def at(nil, _cursor), do: raise "invalid result set"
  def at(_r, _cursor), do: raise "nit not loaded"

  defp init do
    :erlang.load_nif "priv/swift/pg/result", 0
  end
end

defimpl Enumerable, for: Swift.Pg.Result do
  def reduce(_, {:halt,    acc}, _fun), do: {:halted, acc}
  def reduce(r, {:suspend, acc},  fun), do: {:suspended, acc, &reduce(r, &1, fun)}

  def reduce(%Swift.Pg.Result{cursor: count, count: count}, {:cont, acc}, _fun) do
    {:done, acc}
  end

  def reduce(r, {:cont, acc}, fun) do
    reduce(
      %Swift.Pg.Result{result: r.result, count: r.count, cursor: r.cursor + 1},
      fun.(next(r), acc), fun)
  end

  def count(r), do: {:ok, r.count}
  def member?(_, _), do: {:error, "not implemented"}

  defp next(r) do
    Swift.Pg.Result.at(r.result, r.cursor)
  end
end
