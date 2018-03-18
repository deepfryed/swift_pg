#pragma once

#include <erl_nif.h>

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

static inline bool term_to_cstring(ErlNifEnv *env, ERL_NIF_TERM term, void *s, size_t size) {
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
