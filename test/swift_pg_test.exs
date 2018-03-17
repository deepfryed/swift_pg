defmodule SwiftPgTest do
  use ExUnit.Case
  doctest SwiftPg

  test "connects to db" do
    assert {:ok, _ref} = SwiftPg.connect
  end

  test "runs query" do
    assert {:ok, db} = SwiftPg.connect
    assert {:ok, rv} = SwiftPg.query(db, "select 1")
  
    assert rv > 0
  end
end
