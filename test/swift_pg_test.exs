defmodule Swift.Pg.Test do
  use ExUnit.Case
  doctest Swift.Pg

  test "connects to db" do
    assert {:ok, _ref} = Swift.Pg.connect
  end

  test "runs query without parameters" do
    assert {:ok, db} = Swift.Pg.connect
    assert {:ok,  r} = Swift.Pg.exec(db, "select 1")

    assert %Swift.Pg.Result{count: 1, cursor: 0} = r
  end
end
