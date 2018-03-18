#pragma once

// -----------------------------------------------------------------------------
// helper macros
// -----------------------------------------------------------------------------
#define CSTRING_TO_TERM(e, s) cstring_to_term(e, s, strlen(s))
#define SWIFT_PG_ERROR(e, m)  enif_make_tuple2(e, k_error, CSTRING_TO_TERM(e, m))
#define MIN(a, b)             ((a) < (b) ? (a) : (b))
#define MAX(a, b)             ((a) > (b) ? (a) : (b))
