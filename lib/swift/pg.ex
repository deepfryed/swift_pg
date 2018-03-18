defmodule Swift.Pg do
  @moduledoc """
  Postgres driver with libpq bindings.
  """

  alias Swift.Pg.Connection

  def connect, do: Connection.new
  def connect(params = %{}), do: Connection.new(params)

  def exec(db, sql), do: Connection.exec(db, sql)
  def exec(db, sql, bind), do: Connection.exec(db, sql, bind)
end
