#pragma once
#include <erl_nif.h>
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>

ERL_NIF_TERM swift_pg_result_new(ErlNifEnv *env, PGresult *result);
