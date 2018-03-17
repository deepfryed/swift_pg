defmodule Swift.Pg do
  @moduledoc """
  Postgres driver with libpq bindings.
  """

  def connect, do: Swift.Pg.Connection.new
  def connect(params = %{}), do: Swift.Pg.Connection.new(params)

  def exec(db, sql), do: Swift.Pg.Connection.exec(db, sql)
  def exec(db, sql, bind), do: Swift.Pg.Connection.exec(db, sql, bind)

  # TODO
  # def prepare(db, sql)
end
