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

ErlNifResourceType *SWIFT_PG_CONNECTION_RES_TYPE;

static ERL_NIF_TERM k_host;
static ERL_NIF_TERM k_port;
static ERL_NIF_TERM k_user;
static ERL_NIF_TERM k_pass;
static ERL_NIF_TERM k_db;
static ERL_NIF_TERM k_ok;
static ERL_NIF_TERM k_error;

typedef struct {
  PGconn *connection;
  int current_transaction_nesting_level;
  int native_bind_placeholders;
} swift_pg_connection_t;

// -----------------------------------------------------------------------------
// resource allocation
// -----------------------------------------------------------------------------
static void swift_pg_connection_release(ErlNifEnv *env, void *res) {
  swift_pg_connection_t *db = *(swift_pg_connection_t **)res;
  if (db && db->connection) {
    PQfinish(db->connection);
    db->connection = NULL;
  }
  free(db);
}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------
static ERL_NIF_TERM swift_pg_connection_new(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  char info[1024], buffer[512];

  if (argc == 0)
    return SWIFT_PG_ERROR(env, "missing connection parameters");

  if (!enif_is_map(env, argv[0]))
    return SWIFT_PG_ERROR(env, "connection parameters should be a map");

  ERL_NIF_TERM term;

  memset(info, 0, sizeof(info));
  memset(buffer, 0, sizeof(buffer));

  if (enif_get_map_value(env, argv[0], k_host, &term)) {
    if (term_to_cstring(env, term, buffer, sizeof(buffer))) {
      strcat(info, "host=");
      strcat(info, buffer);
    }
    else {
      return SWIFT_PG_ERROR(env, "host must be a binary");
    }
  }

  if (enif_get_map_value(env, argv[0], k_port, &term)) {
    int value;
    if (enif_get_int(env, term, &value)) {
      sprintf(buffer, " port=%d", value);
      strcat(info, buffer);
    }
    else {
      return SWIFT_PG_ERROR(env, "port must be an integer");
    }
  }

  if (enif_get_map_value(env, argv[0], k_user, &term)) {
    if (term_to_cstring(env, term, buffer, sizeof(buffer))) {
      strcat(info, " user=");
      strcat(info, buffer);
    }
    else {
      return SWIFT_PG_ERROR(env, "user must be a binary");
    }
  }

  if (enif_get_map_value(env, argv[0], k_pass, &term)) {
    if (term_to_cstring(env, term, buffer, sizeof(buffer))) {
      strcat(info, " password=");
      strcat(info, buffer);
    }
    else {
      return SWIFT_PG_ERROR(env, "user must be a binary");
    }
  }

  if (enif_get_map_value(env, argv[0], k_db, &term)) {
    if (term_to_cstring(env, term, buffer, sizeof(buffer))) {
      strcat(info, " dbname=");
      strcat(info, buffer);
    }
    else {
      return SWIFT_PG_ERROR(env, "user must be a binary");
    }
  }

  swift_pg_connection_t *db = (swift_pg_connection_t *) malloc(sizeof(swift_pg_connection_t));
  memset(db, 0, sizeof(*db));

  db->connection = PQconnectdb(info);
  if (!db->connection) {
    free(db);
    return SWIFT_PG_ERROR(env, "unable to allocate connection");
  }

  if (PQstatus(db->connection) ==  CONNECTION_BAD) {
    ERL_NIF_TERM error = SWIFT_PG_ERROR(env, PQerrorMessage(db->connection));
    PQfinish(db->connection);
    free(db);
    return error;
  }

  swift_pg_connection_t **db_res = enif_alloc_resource(SWIFT_PG_CONNECTION_RES_TYPE, sizeof(swift_pg_connection_t *));
  if (!db_res)
    return SWIFT_PG_ERROR(env, "unable to allocate resource");
  *db_res = db;

  ERL_NIF_TERM db_term = enif_make_resource(env, db_res);
  enif_release_resource(db_res);

  return enif_make_tuple2(env, k_ok, db_term);
}

static ERL_NIF_TERM swift_pg_connection_exec(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  char sql[4096];
  swift_pg_connection_t *db;
  swift_pg_connection_t **db_res = NULL;

  if (argc != 3)
    return SWIFT_PG_ERROR(env, "requires db resource, sql and bind values (or empty list)");

  if (!enif_get_resource(env, argv[0], SWIFT_PG_CONNECTION_RES_TYPE, (void**)&db_res))
    return SWIFT_PG_ERROR(env, "first argument needs to be db connection resource");

  if (!term_to_cstring(env, argv[1], sql, sizeof(sql)))
    return SWIFT_PG_ERROR(env, "sql should be a valid binary");

  unsigned int length;
  if (!enif_get_list_length(env, argv[2], &length))
    return SWIFT_PG_ERROR(env, "bind values should be a list");

  db = *db_res;
  if (length == 0) {
    ERL_NIF_TERM rv, error;
    PGresult *res = PQexec(db->connection, sql);
    switch (PQresultStatus(res)) {
      case PGRES_FATAL_ERROR:
      case PGRES_NONFATAL_ERROR:
      case PGRES_BAD_RESPONSE:
        error = SWIFT_PG_ERROR(env, PQerrorMessage(db->connection));
        rv = enif_make_tuple2(env, k_error, error);
        break;
      default:
        rv = enif_make_tuple2(env, k_ok, swift_pg_result_new(env, res));
    }

    return rv;
  }
  else {
    return SWIFT_PG_ERROR(env, "TODO bind values");
  }
}

static ErlNifFunc nif_functions[] = {
  {"new",     1, swift_pg_connection_new,  0},
  {"do_exec", 3, swift_pg_connection_exec, 0}
};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info) {
  SWIFT_PG_CONNECTION_RES_TYPE = enif_open_resource_type(
    env,
    NULL,
    "swift_pg_connection",
    swift_pg_connection_release,
    ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER,
    NULL
  );

  k_host  = enif_make_atom(env, "host");
  k_port  = enif_make_atom(env, "port");
  k_user  = enif_make_atom(env, "user");
  k_pass  = enif_make_atom(env, "password");
  k_db    = enif_make_atom(env, "db");
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

ERL_NIF_INIT(Elixir.Swift.Pg.Connection, nif_functions, &load, &reload, &upgrade, &unload)
