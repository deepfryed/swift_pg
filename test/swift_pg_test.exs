defmodule Swift.Pg.Test do
  use ExUnit.Case
  doctest Swift.Pg

  test "connects to db" do
    assert {:ok, _ref} = Swift.Pg.connect
  end

  test "runs query" do
    assert {:ok, db} = Swift.Pg.connect
    assert {:ok, rv} = Swift.Pg.query(db, "select pg_sleep(0.2)")
  
    assert rv > 0
  end
end
