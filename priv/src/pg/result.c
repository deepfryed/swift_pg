#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <erl_nif.h>
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>

#include "macros.h"
#include "result.h"
#include "utils.h"

ErlNifResourceType *SWIFT_PG_RESULT_RES_TYPE;

typedef struct {
  PGresult *result;
} swift_pg_result_t;

static ERL_NIF_TERM k_ok;
static ERL_NIF_TERM k_error;

// -----------------------------------------------------------------------------
// resource allocation
// -----------------------------------------------------------------------------
void swift_pg_result_release(ErlNifEnv *env, void *res) {
  swift_pg_result_t *r = *(swift_pg_result_t **)res;
  PQclear(r->result);
  free(r);
}

// -----------------------------------------------------------------------------
// PRIVATE API
// -----------------------------------------------------------------------------
ERL_NIF_TERM swift_pg_result_new(ErlNifEnv *env, PGresult *result) {
  swift_pg_result_t *r = (swift_pg_result_t *) malloc(sizeof(swift_pg_result_t));
  r->result = result;

  swift_pg_result_t **r_res = enif_alloc_resource(SWIFT_PG_RESULT_RES_TYPE, sizeof(swift_pg_result_t *));
  *r_res = r;

  ERL_NIF_TERM r_term = enif_make_resource(env, r_res);
  enif_release_resource(r_res);
  return r_term;
}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------
static ERL_NIF_TERM swift_pg_result_ntuples(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  swift_pg_result_t *r;
  swift_pg_result_t **r_res = NULL;

  if (!enif_get_resource(env, argv[0], SWIFT_PG_RESULT_RES_TYPE, (void**)&r_res))
    return SWIFT_PG_ERROR(env, "first argument needs to be result resource");

  r = *r_res;
  return enif_make_int(env, PQntuples(r->result));
}

static ERL_NIF_TERM swift_pg_result_at(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  swift_pg_result_t *r;
  swift_pg_result_t **r_res = NULL;

  if (!enif_get_resource(env, argv[0], SWIFT_PG_RESULT_RES_TYPE, (void**)&r_res))
    return SWIFT_PG_ERROR(env, "first argument needs to be result resource");

  int cursor;
  if (!enif_get_int(env, argv[1], &cursor))
    return SWIFT_PG_ERROR(env, "cursor needs to be an integer");

  r = *r_res;

  int length;
  ERL_NIF_TERM ntuple;
  ERL_NIF_TERM tuple = enif_make_new_map(env);

  for (int n = 0; n < PQnfields(r->result); n++) {
    const char *fname = PQfname(r->result, n);
    ERL_NIF_TERM field = enif_make_atom(env, fname);

    if (PQgetisnull(r->result, cursor, n)) {
      enif_make_map_put(env, tuple, field, enif_make_atom(env, "nil"), &ntuple);
      tuple = ntuple;
    }
    else {
      length = PQgetlength(r->result, cursor, n);
      enif_make_map_put(
        env,
        tuple,
        field,
        cstring_to_term(env, PQgetvalue(r->result, cursor, n), length),
        &ntuple
      );
      tuple = ntuple;
    }
  }

  return tuple;
}

//static ERL_NIF_TERM swift_pg_result_fields(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
//}

static ErlNifFunc nif_functions[] = {
  {"ntuples",  1, swift_pg_result_ntuples, 0},
  {"at",       2, swift_pg_result_at,      0}
};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info) {
  SWIFT_PG_RESULT_RES_TYPE = enif_open_resource_type(
    env,
    NULL,
    "swift_pg_result",
    swift_pg_result_release,
    ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER,
    NULL
  );

  k_ok    = enif_make_atom(env, "ok");
  k_error = enif_make_atom(env, "error");

  return 0;
}

static int reload(ErlNifEnv *env, void **priv, ERL_NIF_TERM info) {
  return 0;
}

static int upgrade(ErlNifEnv *env, void **priv, void **old_priv, ERL_NIF_TERM info) {
  return 0;
}

static void unload(ErlNifEnv *env, void *priv) {
}

ERL_NIF_INIT(Elixir.Swift.Pg.Result, nif_functions, &load, &reload, &upgrade, &unload)
