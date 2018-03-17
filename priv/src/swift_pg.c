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

ErlNifResourceType *SWIFT_DB_RES_TYPE;

static ERL_NIF_TERM k_host;
static ERL_NIF_TERM k_port;
static ERL_NIF_TERM k_user;
static ERL_NIF_TERM k_pass;
static ERL_NIF_TERM k_db;

typedef struct {
  PGconn *connection;
  int current_transaction_nesting_level;
  int native_bind_placeholders;
} swift_db_t;

// -----------------------------------------------------------------------------
// helper macros
// -----------------------------------------------------------------------------
#define CSTRING_TO_TERM(e, s) cstring_to_term(e, s, strlen(s))
#define SWIFT_PG_ERROR(e, m)  enif_make_tuple2(e, enif_make_atom(e, "error"), CSTRING_TO_TERM(e, m))
#define MIN(a, b)             ((a) < (b) ? (a) : (b))
#define MAX(a, b)             ((a) > (b) ? (a) : (b))
// -----------------------------------------------------------------------------
// term & typecasts
// -----------------------------------------------------------------------------

static inline ERL_NIF_TERM cstring_to_term(ErlNifEnv *env, const char *s, size_t size) {
  ERL_NIF_TERM term;
  unsigned char *bin;
  bin = enif_make_new_binary(env, size, &term);
  memcpy(bin, s, size);
  return term;
}

static bool term_to_cstring(ErlNifEnv *env, ERL_NIF_TERM term, void *s, size_t size) {
  ErlNifBinary binary;
  if (enif_inspect_binary(env, term, &binary)) {
    size = MIN(binary.size, size - 1);
    memcpy(s, binary.data, size);
    ((char*)s)[size] = 0;
    return true;
  }
  else {
    return false;
  }
}

// -----------------------------------------------------------------------------
// resource allocation
// -----------------------------------------------------------------------------
void swift_pg_release(ErlNifEnv *env, void *res) {
  swift_db_t *db = *(swift_db_t **)res;
  if (db && db->connection) {
    PQfinish(db->connection);
    db->connection = NULL;
  }
  free(db);
}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------
ERL_NIF_TERM swift_pg_connect(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
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

  swift_db_t *db = (swift_db_t *) malloc(sizeof(swift_db_t));
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

  swift_db_t **db_res = enif_alloc_resource(SWIFT_DB_RES_TYPE, sizeof(swift_db_t *));
  if (!db_res)
    return SWIFT_PG_ERROR(env, "unable to allocate resource");
  *db_res = db;

  ERL_NIF_TERM db_term = enif_make_resource(env, db_res);
  enif_release_resource(db_res);

  return enif_make_tuple2(env, enif_make_atom(env, "ok"), db_term);
}

static ErlNifFunc nif_functions[] = {
  {"connect", 1, swift_pg_connect, 0}
//  {"query",   3, swift_pg_query,   0}
};

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info) {
  SWIFT_DB_RES_TYPE = enif_open_resource_type(
    env,
    NULL,
    "swift_pg",
    swift_pg_release,
    ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER,
    NULL
  );

  k_host = enif_make_atom(env, "host");
  k_port = enif_make_atom(env, "port");
  k_user = enif_make_atom(env, "user");
  k_pass = enif_make_atom(env, "password");
  k_db   = enif_make_atom(env, "db");

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

ERL_NIF_INIT(Elixir.SwiftPg, nif_functions, &load, &reload, &upgrade, &unload)
