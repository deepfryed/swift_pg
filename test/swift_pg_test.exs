defmodule SwiftPgTest do
  use ExUnit.Case
  doctest SwiftPg

  test "connects to db" do
    assert {:ok, _ref} = SwiftPg.connect
  end
end
