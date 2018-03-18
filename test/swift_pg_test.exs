defmodule Swift.Pg.Test do
  use ExUnit.Case
  doctest Swift.Pg

  setup do
    {:ok, conn} = Swift.Pg.connect
    {:ok, conn: conn}
  end

  test "connects to db", %{conn: conn} do
    assert conn
  end

  test "runs query without parameters", %{conn: conn} do
    assert {:ok, r} = Swift.Pg.exec(conn, "select 1")
    assert %Swift.Pg.Result{count: 1, cursor: 0} = r
  end

  test "runs query with parameters", %{conn: conn} do
    assert {:ok, r} = Swift.Pg.exec(conn, "select 2 > $1", [1])
    assert %Swift.Pg.Result{count: 1, cursor: 0} = r
  end
end
