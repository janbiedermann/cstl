/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H /* Development inclusion - ignore line */
#include "000 header.h"             /* Development inclusion - ignore line */
#include "003 atomics.h"            /* Development inclusion - ignore line */
#include "004 bitwise.h"            /* Development inclusion - ignore line */
#include "005 riskyhash.h"          /* Development inclusion - ignore line */
#include "006 atol.h"               /* Development inclusion - ignore line */
#include "008 json.h"               /* Development inclusion - ignore line */
#include "021 array.h"              /* Development inclusion - ignore line */
#include "022 hashmap.h"            /* Development inclusion - ignore line */
#include "023 string.h"             /* Development inclusion - ignore line */
#include "030 reference counter.h"  /* Development inclusion - ignore line */
#include "050 cleanup.h"            /* Development inclusion - ignore line */
#endif                              /* Development inclusion - ignore line */
/* *****************************************************************************














                          FIOBJ - soft (dynamic) types









FIOBJ - dynamic types

These are dynamic types that use pointer tagging for fast type identification.

Pointer tagging on 64 bit systems allows for 3 bits at the lower bits. On most
32 bit systems this is also true due to allocator alignment. When in doubt, use
the provided custom allocator.

To keep the 64bit memory address alignment on 32bit systems, a 32bit metadata
integer is added when a virtual function table is missing. This doesn't effect
memory consumption on 64 bit systems and uses 4 bytes on 32 bit systems.

Note: this code is placed at the end of the STL file, since it leverages most of
the SLT features and could be affected by their inclusion.
***************************************************************************** */
#if defined(FIO_FIOBJ) && !defined(H___FIOBJ_H) && !defined(FIO_TEST_CSTL)
#define H___FIOBJ_H

/* *****************************************************************************
FIOBJ compilation settings (type names and JSON nesting limits).

Type Naming Macros for FIOBJ types. By default, results in:
- fiobj_true()
- fiobj_false()
- fiobj_null()
- fiobj_num_new() ... (etc')
- fiobj_float_new() ... (etc')
- fiobj_str_new() ... (etc')
- fiobj_array_new() ... (etc')
- fiobj_hash_new() ... (etc')
***************************************************************************** */

#define FIOBJ___NAME_TRUE true
#define FIOBJ___NAME_FALSE false
#define FIOBJ___NAME_NULL null
#define FIOBJ___NAME_NUMBER num
#define FIOBJ___NAME_FLOAT float
#define FIOBJ___NAME_STRING str
#define FIOBJ___NAME_ARRAY array
#define FIOBJ___NAME_HASH hash

#ifndef FIOBJ_MAX_NESTING
/**
 * Sets the limit on nesting level transversal by recursive functions.
 *
 * This effects JSON output / input and the `fiobj_each2` function since they
 * are recursive.
 *
 * HOWEVER: this value will NOT effect the recursive `fiobj_free` which could
 * (potentially) expload the stack if given melformed input such as cyclic data
 * structures.
 *
 * Values should be less than 32K.
 */
#define FIOBJ_MAX_NESTING 512
#endif

/* make sure roundtrips work */
#ifndef JSON_MAX_DEPTH
#define JSON_MAX_DEPTH FIOBJ_MAX_NESTING
#endif
/* *****************************************************************************
General Requirements / Macros
***************************************************************************** */

#define FIO_ATOL 1
#define FIO_ATOMIC 1
#include __FILE__

#ifdef FIOBJ_EXTERN
#define FIOBJ_FUNC
#define FIOBJ_IFUNC
#define FIOBJ_EXTERN_OBJ extern
#define FIOBJ_EXTERN_OBJ_IMP __attribute__((weak))

#else /* FIO_EXTERN */
#define FIOBJ_FUNC static __attribute__((unused))
#define FIOBJ_IFUNC static inline __attribute__((unused))
#define FIOBJ_EXTERN_OBJ static __attribute__((unused))
#define FIOBJ_EXTERN_OBJ_IMP static __attribute__((unused))
#ifndef FIOBJ_EXTERN_COMPLETE /* force implementation, emitting static data */
#define FIOBJ_EXTERN_COMPLETE 2
#endif /* FIOBJ_EXTERN_COMPLETE */

#endif /* FIO_EXTERN */

#ifdef FIO_LOG_PRINT__
#define FIOBJ_LOG_PRINT__(...) FIO_LOG_PRINT__(__VA_ARGS__)
#else
#define FIOBJ_LOG_PRINT__(...)
#endif

#ifdef __cplusplus /* C++ doesn't allow declarations for static variables */
#undef FIOBJ_EXTERN_OBJ
#undef FIOBJ_EXTERN_OBJ_IMP
#define FIOBJ_EXTERN_OBJ extern
#define FIOBJ_EXTERN_OBJ_IMP __attribute__((weak))
#endif

/* *****************************************************************************
Debugging / Leak Detection
***************************************************************************** */
#if (TEST || DEBUG) && !defined(FIOBJ_MARK_MEMORY)
#define FIOBJ_MARK_MEMORY 1
#endif

#if FIOBJ_MARK_MEMORY
size_t __attribute__((weak)) FIOBJ_MARK_MEMORY_ALLOC_COUNTER;
size_t __attribute__((weak)) FIOBJ_MARK_MEMORY_FREE_COUNTER;
#define FIOBJ_MARK_MEMORY_ALLOC()                                              \
  fio_atomic_add(&FIOBJ_MARK_MEMORY_ALLOC_COUNTER, 1)
#define FIOBJ_MARK_MEMORY_FREE()                                               \
  fio_atomic_add(&FIOBJ_MARK_MEMORY_FREE_COUNTER, 1)
#define FIOBJ_MARK_MEMORY_PRINT()                                              \
  FIOBJ_LOG_PRINT__(                                                           \
      ((FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER)     \
           ? 4 /* FIO_LOG_LEVEL_INFO */                                        \
           : 3 /* FIO_LOG_LEVEL_WARNING */),                                   \
      ((FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER)     \
           ? "INFO: total FIOBJ allocations: %zu (%zu/%zu)"                    \
           : "WARNING: LEAKED! FIOBJ allocations: %zu (%zu/%zu)"),             \
      FIOBJ_MARK_MEMORY_ALLOC_COUNTER - FIOBJ_MARK_MEMORY_FREE_COUNTER,        \
      FIOBJ_MARK_MEMORY_FREE_COUNTER, FIOBJ_MARK_MEMORY_ALLOC_COUNTER)
#define FIOBJ_MARK_MEMORY_ENABLED 1

#else

#define FIOBJ_MARK_MEMORY_ALLOC_COUNTER 0 /* when testing unmarked FIOBJ */
#define FIOBJ_MARK_MEMORY_FREE_COUNTER 0  /* when testing unmarked FIOBJ */
#define FIOBJ_MARK_MEMORY_ALLOC()
#define FIOBJ_MARK_MEMORY_FREE()
#define FIOBJ_MARK_MEMORY_PRINT()
#define FIOBJ_MARK_MEMORY_ENABLED 0
#endif

/* *****************************************************************************
The FIOBJ Type
***************************************************************************** */

/** Use the FIOBJ type for dynamic types. */
typedef struct FIOBJ_s {
  struct FIOBJ_s *compiler_validation_type;
} * FIOBJ;

/** FIOBJ type enum for common / primitive types. */
typedef enum {
  FIOBJ_T_NUMBER = 0x01, /* 0b001 3 bits taken for small numbers */
  FIOBJ_T_PRIMITIVE = 2, /* 0b010 a lonely second bit signifies a primitive */
  FIOBJ_T_STRING = 3,    /* 0b011 */
  FIOBJ_T_ARRAY = 4,     /* 0b100 */
  FIOBJ_T_HASH = 5,      /* 0b101 */
  FIOBJ_T_FLOAT = 6,     /* 0b110 */
  FIOBJ_T_OTHER = 7,     /* 0b111 dynamic type - test content */
} fiobj_class_en;

#define FIOBJ_T_NULL 2   /* 0b010 a lonely second bit signifies a primitive */
#define FIOBJ_T_TRUE 18  /* 0b010 010 - primitive value */
#define FIOBJ_T_FALSE 34 /* 0b100 010 - primitive value */

/** Use the macros to avoid future API changes. */
#define FIOBJ_TYPE(o) fiobj_type(o)
/** Use the macros to avoid future API changes. */
#define FIOBJ_TYPE_IS(o, type) (fiobj_type(o) == type)
/** Identifies an invalid type identifier (returned from FIOBJ_TYPE(o) */
#define FIOBJ_T_INVALID 0
/** Identifies an invalid object */
#define FIOBJ_INVALID 0
/** Tests if the object is (probably) a valid FIOBJ */
#define FIOBJ_IS_INVALID(o) (((uintptr_t)(o)&7UL) == 0)
#define FIOBJ_TYPE_CLASS(o) ((fiobj_class_en)(((uintptr_t)o) & 7UL))
#define FIOBJ_PTR_UNTAG(o) ((uintptr_t)o & (~7ULL))
/** Returns an objects type. This isn't limited to known types. */
FIO_IFUNC size_t fiobj_type(FIOBJ o);

/* *****************************************************************************
FIOBJ Memory Management
***************************************************************************** */

/** Increases an object's reference count (or copies) and returns it. */
FIO_IFUNC FIOBJ fiobj_dup(FIOBJ o);

/** Decreases an object's reference count or frees it. */
FIO_IFUNC void fiobj_free(FIOBJ o);

/* *****************************************************************************
FIOBJ Data / Info
***************************************************************************** */

/** Compares two objects. */
FIO_IFUNC unsigned char FIO_NAME_BL(fiobj, eq)(FIOBJ a, FIOBJ b);

/** Returns a temporary String representation for any FIOBJ object. */
FIO_IFUNC fio_str_info_s FIO_NAME2(fiobj, cstr)(FIOBJ o);

/** Returns an integer representation for any FIOBJ object. */
FIO_IFUNC intptr_t FIO_NAME2(fiobj, i)(FIOBJ o);

/** Returns a float (double) representation for any FIOBJ object. */
FIO_IFUNC double FIO_NAME2(fiobj, f)(FIOBJ o);

/* *****************************************************************************
FIOBJ Containers (iteration)
***************************************************************************** */

/**
 * Performs a task for each element held by the FIOBJ object.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the "stop" position - the number of elements processed + `start_at`.
 */
FIO_SFUNC uint32_t fiobj_each1(FIOBJ o, int32_t start_at,
                               int (*task)(FIOBJ child, void *arg), void *arg);

/**
 * Performs a task for the object itself and each element held by the FIOBJ
 * object or any of it's elements (a deep task).
 *
 * The order of performance is by order of appearance, as if all nesting levels
 * were flattened.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the number of elements processed.
 */
FIOBJ_FUNC uint32_t fiobj_each2(FIOBJ o, int (*task)(FIOBJ child, void *arg),
                                void *arg);

/* *****************************************************************************
FIOBJ Primitives (NULL, True, False)
***************************************************************************** */

/** Returns the `true` primitive. */
FIO_IFUNC FIOBJ FIO_NAME(fiobj, FIOBJ___NAME_TRUE)(void) {
  return (FIOBJ)(FIOBJ_T_TRUE);
}

/** Returns the `false` primitive. */
FIO_IFUNC FIOBJ FIO_NAME(fiobj, FIOBJ___NAME_FALSE)(void) {
  return (FIOBJ)(FIOBJ_T_FALSE);
}

/** Returns the `nil` / `null` primitive. */
FIO_IFUNC FIOBJ FIO_NAME(fiobj, FIOBJ___NAME_NULL)(void) {
  return (FIOBJ)(FIOBJ_T_NULL);
}

/* *****************************************************************************
FIOBJ Type - Extendability (FIOBJ_T_OTHER)
***************************************************************************** */

/** FIOBJ types can be extended using virtual function tables. */
typedef struct {
  /**
   * MUST return a unique number to identify object type.
   *
   * Numbers (type IDs) under 100 are reserved. Numbers under 40 are illegal.
   */
  size_t type_id;
  /** Test for equality between two objects with the same `type_id` */
  unsigned char (*is_eq)(FIOBJ restrict a, FIOBJ restrict b);
  /** Converts an object to a String */
  fio_str_info_s (*to_s)(FIOBJ o);
  /** Converts an object to an integer */
  intptr_t (*to_i)(FIOBJ o);
  /** Converts an object to a double */
  double (*to_f)(FIOBJ o);
  /** Returns the number of exposed elements held by the object, if any. */
  uint32_t (*count)(FIOBJ o);
  /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
  uint32_t (*each1)(FIOBJ o, int32_t start_at,
                    int (*task)(FIOBJ child, void *arg), void *arg);
  /**
   * Decreases the reference count and/or frees the object, calling `free2` for
   * any nested objects.
   *
   * Returns 0 if the object is still alive or 1 if the object was freed. The
   * return value is currently ignored, but this might change in the future.
   */
  int (*free2)(FIOBJ o);
} FIOBJ_class_vtable_s;

FIOBJ_EXTERN_OBJ const FIOBJ_class_vtable_s FIOBJ___OBJECT_CLASS_VTBL;

#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_NAME fiobj_object
#define FIO_REF_TYPE void *
#define FIO_REF_METADATA const FIOBJ_class_vtable_s *
#define FIO_REF_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___OBJECT_CLASS_VTBL;                                            \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REF_METADATA_DESTROY(m)                                            \
  do {                                                                         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_PTR_TAG(p) ((uintptr_t)p | FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p) FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE FIOBJ
#include __FILE__

/* *****************************************************************************
FIOBJ Integers
***************************************************************************** */

/** Creates a new Number object. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(intptr_t i);

/** Reads the number from a FIOBJ Number. */
FIO_IFUNC intptr_t FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(FIOBJ i);

/** Reads the number from a FIOBJ Number, fitting it in a double. */
FIO_IFUNC double FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), f)(FIOBJ i);

/** Returns a String representation of the number (in base 10). */
FIOBJ_FUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER),
                                    cstr)(FIOBJ i);

/** Frees a FIOBJ number. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), free)(FIOBJ i);

FIOBJ_EXTERN_OBJ const FIOBJ_class_vtable_s FIOBJ___NUMBER_CLASS_VTBL;

/* *****************************************************************************
FIOBJ Floats
***************************************************************************** */

/** Creates a new Float (double) object. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(double i);

/** Reads the number from a FIOBJ Float rounding it to an integer. */
FIO_IFUNC intptr_t FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), i)(FIOBJ i);

/** Reads the value from a FIOBJ Float, as a double. */
FIO_IFUNC double FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(FIOBJ i);

/** Returns a String representation of the float. */
FIOBJ_FUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT),
                                    cstr)(FIOBJ i);

/** Frees a FIOBJ Float. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), free)(FIOBJ i);

FIOBJ_EXTERN_OBJ const FIOBJ_class_vtable_s FIOBJ___FLOAT_CLASS_VTBL;

/* *****************************************************************************
FIOBJ Strings
***************************************************************************** */

#define FIO_STR_NAME FIO_NAME(fiobj, FIOBJ___NAME_STRING)
#define FIO_STR_OPTIMIZE_EMBEDDED 1
#define FIO_REF_NAME FIO_NAME(fiobj, FIOBJ___NAME_STRING)
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_DESTROY(s)                                                     \
  do {                                                                         \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), destroy)((FIOBJ)&s);        \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_REF_INIT(s_)                                                       \
  do {                                                                         \
    s_ = (FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s))FIO_STR_INIT;      \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REF_METADATA uint32_t /* for 32bit system padding */
#define FIO_PTR_TAG(p) ((uintptr_t)p | FIOBJ_T_STRING)
#define FIO_PTR_UNTAG(p) FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE FIOBJ
#include __FILE__

/* Creates a new FIOBJ string object, copying the data to the new string. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                         new_cstr)(const char *ptr, size_t len) {
  FIOBJ s = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_ASSERT_ALLOC(FIOBJ_PTR_UNTAG(s));
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(s, ptr, len);
  return s;
}

/* Creates a new FIOBJ string object with (at least) the requested capacity. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                         new_buf)(size_t capa) {
  FIOBJ s = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_ASSERT_ALLOC(FIOBJ_PTR_UNTAG(s));
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), reserve)(s, capa);
  return s;
}

/* Creates a new FIOBJ string object, copying the origin (`fiobj2cstr`). */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                         new_copy)(FIOBJ original) {
  FIOBJ s = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_ASSERT_ALLOC(FIOBJ_PTR_UNTAG(s));
  fio_str_info_s i = FIO_NAME2(fiobj, cstr)(original);
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(s, i.buf, i.len);
  return s;
}

/** Returns information about the string. Same as fiobj_str_info(). */
FIO_IFUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                                   cstr)(FIOBJ s) {
  return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), info)(s);
}

/**
 * Creates a temporary FIOBJ String object on the stack.
 *
 * String data might be allocated dynamically.
 */
#define FIOBJ_STR_TEMP_VAR(str_name)                                           \
  struct {                                                                     \
    uint64_t i1;                                                               \
    uint64_t i2;                                                               \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;                       \
  } FIO_NAME(str_name, __auto_mem_tmp) = {                                     \
      0x7f7f7f7f7f7f7f7fULL, 0x7f7f7f7f7f7f7f7fULL, FIO_STR_INIT};             \
  FIOBJ str_name =                                                             \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |         \
              FIOBJ_T_STRING);

/**
 * Creates a temporary FIOBJ String object on the stack, initialized with a
 * static string.
 *
 * String data might be allocated dynamically.
 */
#define FIOBJ_STR_TEMP_VAR_STATIC(str_name, buf_, len_)                        \
  struct {                                                                     \
    uint64_t i1;                                                               \
    uint64_t i2;                                                               \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;                       \
  } FIO_NAME(str_name,                                                         \
             __auto_mem_tmp) = {0x7f7f7f7f7f7f7f7fULL, 0x7f7f7f7f7f7f7f7fULL,  \
                                FIO_STR_INIT_STATIC2((buf_), (len_))};         \
  FIOBJ str_name =                                                             \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |         \
              FIOBJ_T_STRING);

/**
 * Creates a temporary FIOBJ String object on the stack, initialized with a
 * static string.
 *
 * String data might be allocated dynamically.
 */
#define FIOBJ_STR_TEMP_VAR_EXISTING(str_name, buf_, len_, capa_)               \
  struct {                                                                     \
    uint64_t i1;                                                               \
    uint64_t i2;                                                               \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), s) s;                       \
  } FIO_NAME(str_name, __auto_mem_tmp) = {                                     \
      0x7f7f7f7f7f7f7f7fULL, 0x7f7f7f7f7f7f7f7fULL,                            \
      FIO_STR_INIT_EXISTING((buf_), (len_), (capa_))};                         \
  FIOBJ str_name =                                                             \
      (FIOBJ)(((uintptr_t) & (FIO_NAME(str_name, __auto_mem_tmp).s)) |         \
              FIOBJ_T_STRING);

/** Resets a temporary FIOBJ String, freeing and any resources allocated. */
#define FIOBJ_STR_TEMP_DESTROY(str_name)                                       \
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), destroy)(str_name);

/* *****************************************************************************
FIOBJ Arrays
***************************************************************************** */

#define FIO_ARRAY_NAME FIO_NAME(fiobj, FIOBJ___NAME_ARRAY)
#define FIO_REF_NAME FIO_NAME(fiobj, FIOBJ___NAME_ARRAY)
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_DESTROY(a)                                                     \
  do {                                                                         \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), destroy)((FIOBJ)&a);         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_REF_INIT(a)                                                        \
  do {                                                                         \
    a = (FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), s))FIO_ARRAY_INIT;      \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REF_METADATA uint32_t /* for 32bit system padding */
#define FIO_ARRAY_TYPE FIOBJ
#define FIO_ARRAY_TYPE_CMP(a, b) FIO_NAME_BL(fiobj, eq)((a), (b))
#define FIO_ARRAY_TYPE_DESTROY(o) fiobj_free(o)
#define FIO_ARRAY_TYPE_CONCAT_COPY(dest, obj)                                  \
  do {                                                                         \
    dest = fiobj_dup(obj);                                                     \
  } while (0)
#define FIO_PTR_TAG(p) ((uintptr_t)p | FIOBJ_T_ARRAY)
#define FIO_PTR_UNTAG(p) FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE FIOBJ
#include __FILE__

/* *****************************************************************************
FIOBJ Hash Maps
***************************************************************************** */

#define FIO_MAP_NAME FIO_NAME(fiobj, FIOBJ___NAME_HASH)
#define FIO_REF_NAME FIO_NAME(fiobj, FIOBJ___NAME_HASH)
#define FIO_REF_CONSTRUCTOR_ONLY 1
#define FIO_REF_DESTROY(a)                                                     \
  do {                                                                         \
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), destroy)((FIOBJ)&a);          \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_REF_INIT(a)                                                        \
  do {                                                                         \
    a = (FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), s))FIO_MAP_INIT;         \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REF_METADATA uint32_t /* for 32bit system padding */
#define FIO_MAP_TYPE FIOBJ
#define FIO_MAP_TYPE_DESTROY(o) fiobj_free(o)
#define FIO_MAP_KEY FIOBJ
#define FIO_MAP_KEY_CMP(a, b) FIO_NAME_BL(fiobj, eq)((a), (b))
#define FIO_MAP_KEY_COPY(dest, o) (dest = fiobj_dup(o))
#define FIO_MAP_KEY_DESTROY(o) fiobj_free(o)
#define FIO_PTR_TAG(p) ((uintptr_t)p | FIOBJ_T_HASH)
#define FIO_PTR_UNTAG(p) FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE FIOBJ
#include __FILE__

/** Calculates an object's hash value for a specific hash map object. */
FIO_IFUNC uint64_t FIO_NAME2(fiobj, hash)(FIOBJ target_hash, FIOBJ object_key);

/** Inserts a value to a hash map, with a default hash value calculation. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         set2)(FIOBJ hash, FIOBJ key, FIOBJ value);

/** Finds a value in a hash map, with a default hash value calculation. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(FIOBJ hash,
                                                                   FIOBJ key);

/** Removes a value from a hash map, with a default hash value calculation. */
FIO_IFUNC int FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                       remove2)(FIOBJ hash, FIOBJ key, FIOBJ *old);

/**
 * Sets a String value in a hash map, allocating the String and automatically
 * calculating the hash value.
 */
FIO_IFUNC
FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
               set3)(FIOBJ hash, const char *key, size_t len, FIOBJ value);

/**
 * Finds a String value in a hash map, using a temporary String and
 * automatically calculating the hash value.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         get3)(FIOBJ hash, const char *buf, size_t len);

/**
 * Removes a String value in a hash map, using a temporary String and
 * automatically calculating the hash value.
 */
FIO_IFUNC int FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                       remove3)(FIOBJ hash, const char *buf, size_t len,
                                FIOBJ *old);

/* *****************************************************************************
FIOBJ JSON support
***************************************************************************** */

/**
 * Returns a JSON valid FIOBJ String, representing the object.
 *
 * If `dest` is an existing String, the formatted JSON data will be appended to
 * the existing string.
 */
FIO_IFUNC FIOBJ FIO_NAME2(fiobj, json)(FIOBJ dest, FIOBJ o, uint8_t beautify);

/**
 * Updates a Hash using JSON data.
 *
 * Parsing errors and non-dictionary object JSON data are silently ignored,
 * attempting to update the Hash as much as possible before any errors
 * encountered.
 *
 * Conflicting Hash data is overwritten (preferring the new over the old).
 *
 * Returns the number of bytes consumed. On Error, 0 is returned and no data is
 * consumed.
 */
FIOBJ_FUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                           update_json)(FIOBJ hash, fio_str_info_s str);

/** Helper function, calls `fiobj_hash_update_json` with string information */
FIO_IFUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                          update_json2)(FIOBJ hash, char *ptr, size_t len);

/**
 * Parses a C string for JSON data. If `consumed` is not NULL, the `size_t`
 * variable will contain the number of bytes consumed before the parser stopped
 * (due to either error or end of a valid JSON data segment).
 *
 * Returns a FIOBJ object matching the JSON valid C string `str`.
 *
 * If the parsing failed (no complete valid JSON data) `FIOBJ_INVALID` is
 * returned.
 */
FIOBJ_FUNC FIOBJ fiobj_json_parse(fio_str_info_s str, size_t *consumed);

/** Helper macro, calls `fiobj_json_parse` with string information */
#define fiobj_json_parse2(data_, len_, consumed)                               \
  fiobj_json_parse((fio_str_info_s){.buf = data_, .len = len_}, consumed)

/* *****************************************************************************







FIOBJ - Implementation - Inline / Macro like fucntions







***************************************************************************** */

/* *****************************************************************************
The FIOBJ Type
***************************************************************************** */

/** Returns an objects type. This isn't limited to known types. */
FIO_IFUNC size_t fiobj_type(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch ((uintptr_t)(o)) {
    case FIOBJ_T_NULL:
      return FIOBJ_T_NULL;
    case FIOBJ_T_TRUE:
      return FIOBJ_T_TRUE;
    case FIOBJ_T_FALSE:
      return FIOBJ_T_FALSE;
    };
    return FIOBJ_T_INVALID;
  case FIOBJ_T_NUMBER:
    return FIOBJ_T_NUMBER;
  case FIOBJ_T_FLOAT:
    return FIOBJ_T_FLOAT;
  case FIOBJ_T_STRING:
    return FIOBJ_T_STRING;
  case FIOBJ_T_ARRAY:
    return FIOBJ_T_ARRAY;
  case FIOBJ_T_HASH:
    return FIOBJ_T_HASH;
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->type_id;
  }
  if (!o)
    return FIOBJ_T_NULL;
  return FIOBJ_T_INVALID;
}

/* *****************************************************************************
FIOBJ Memory Management
***************************************************************************** */

/** Increases an object's reference count (or copies) and returns it. */
FIO_IFUNC FIOBJ fiobj_dup(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fallthrough */
  case FIOBJ_T_NUMBER:    /* fallthrough */
  case FIOBJ_T_FLOAT:     /* fallthrough */
    return o;
  case FIOBJ_T_STRING: /* fallthrough */
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), up_ref)(o);
    break;
  case FIOBJ_T_ARRAY: /* fallthrough */
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), up_ref)(o);
    break;
  case FIOBJ_T_HASH: /* fallthrough */
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), up_ref)(o);
    break;
  case FIOBJ_T_OTHER: /* fallthrough */
    fiobj_object_up_ref(o);
  }
  return o;
}

/** Decreases an object's reference count or frees it. */
FIO_IFUNC void fiobj_free(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fallthrough */
  case FIOBJ_T_NUMBER:    /* fallthrough */
  case FIOBJ_T_FLOAT:
    return;
  case FIOBJ_T_STRING:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), free)(o);
    return;
  case FIOBJ_T_ARRAY:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), free)(o);
    return;
  case FIOBJ_T_HASH:
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), free)(o);
    return;
  case FIOBJ_T_OTHER:
    (*fiobj_object_metadata(o))->free2(o);
    return;
  }
}

/* *****************************************************************************
FIOBJ Data / Info
***************************************************************************** */

/** Internal: compares two nestable objects. */
FIOBJ_FUNC unsigned char fiobj___test_eq_nested(FIOBJ restrict a,
                                                FIOBJ restrict b);

/** Compares two objects. */
FIO_IFUNC unsigned char FIO_NAME_BL(fiobj, eq)(FIOBJ a, FIOBJ b) {
  if (a == b)
    return 1;
  if (FIOBJ_TYPE_CLASS(a) != FIOBJ_TYPE_CLASS(b))
    return 0;
  switch (FIOBJ_TYPE_CLASS(a)) {
  case FIOBJ_T_PRIMITIVE:
  case FIOBJ_T_NUMBER: /* fallthrough */
  case FIOBJ_T_FLOAT:  /* fallthrough */
    return a == b;
  case FIOBJ_T_STRING:
    return FIO_NAME_BL(FIO_NAME(fiobj, FIOBJ___NAME_STRING), eq)(a, b);
  case FIOBJ_T_ARRAY:
    return fiobj___test_eq_nested(a, b);
  case FIOBJ_T_HASH:
    return fiobj___test_eq_nested(a, b);
  case FIOBJ_T_OTHER:
    if ((*fiobj_object_metadata(a))->count(a) ||
        (*fiobj_object_metadata(b))->count(b)) {
      if ((*fiobj_object_metadata(a))->count(a) !=
          (*fiobj_object_metadata(b))->count(b))
        return 0;
      return fiobj___test_eq_nested(a, b);
    }
    return (*fiobj_object_metadata(a))->type_id ==
               (*fiobj_object_metadata(b))->type_id &&
           (*fiobj_object_metadata(a))->is_eq(a, b);
  }
  return 0;
}

#define FIOBJ2CSTR_BUFFER_LIMIT 4096
__thread char __attribute__((weak))
fiobj___2cstr___buffer__perthread[FIOBJ2CSTR_BUFFER_LIMIT];

/** Returns a temporary String representation for any FIOBJ object. */
FIO_IFUNC fio_str_info_s FIO_NAME2(fiobj, cstr)(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch ((uintptr_t)(o)) {
    case FIOBJ_T_NULL:
      return (fio_str_info_s){.buf = (char *)"null", .len = 4};
    case FIOBJ_T_TRUE:
      return (fio_str_info_s){.buf = (char *)"true", .len = 4};
    case FIOBJ_T_FALSE:
      return (fio_str_info_s){.buf = (char *)"false", .len = 5};
    };
    return (fio_str_info_s){.buf = (char *)""};
  case FIOBJ_T_NUMBER:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), cstr)(o);
  case FIOBJ_T_FLOAT:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), cstr)(o);
  case FIOBJ_T_STRING:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), info)(o);
  case FIOBJ_T_ARRAY: /* fallthrough */
  case FIOBJ_T_HASH: {
    FIOBJ j = FIO_NAME2(fiobj, json)(FIOBJ_INVALID, o, 0);
    if (!j || FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(j) >=
                  FIOBJ2CSTR_BUFFER_LIMIT) {
      fiobj_free(j);
      return (fio_str_info_s){.buf = (FIOBJ_TYPE_CLASS(o) == FIOBJ_T_ARRAY
                                          ? (char *)"[...]"
                                          : (char *)"{...}"),
                              .len = 5};
    }
    fio_str_info_s i = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), info)(j);
    memcpy(fiobj___2cstr___buffer__perthread, i.buf, i.len + 1);
    fiobj_free(j);
    i.buf = fiobj___2cstr___buffer__perthread;
    return i;
  }
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->to_s(o);
  }
  if (!o)
    return (fio_str_info_s){.buf = (char *)"null", .len = 4};
  return (fio_str_info_s){.buf = (char *)""};
}

/** Returns an integer representation for any FIOBJ object. */
FIO_IFUNC intptr_t FIO_NAME2(fiobj, i)(FIOBJ o) {
  fio_str_info_s tmp;
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch ((uintptr_t)(o)) {
    case FIOBJ_T_NULL:
      return 0;
    case FIOBJ_T_TRUE:
      return 1;
    case FIOBJ_T_FALSE:
      return 0;
    };
    return -1;
  case FIOBJ_T_NUMBER:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(o);
  case FIOBJ_T_FLOAT:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), i)(o);
  case FIOBJ_T_STRING:
    tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), info)(o);
    if (!tmp.len)
      return 0;
    return fio_atol(&tmp.buf);
  case FIOBJ_T_ARRAY:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
  case FIOBJ_T_HASH:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->to_i(o);
  }
  if (!o)
    return 0;
  return -1;
}

/** Returns a float (double) representation for any FIOBJ object. */
FIO_IFUNC double FIO_NAME2(fiobj, f)(FIOBJ o) {
  fio_str_info_s tmp;
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch ((uintptr_t)(o)) {
    case FIOBJ_T_FALSE: /* fallthrough */
    case FIOBJ_T_NULL:
      return 0.0;
    case FIOBJ_T_TRUE:
      return 1.0;
    };
    return -1.0;
  case FIOBJ_T_NUMBER:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), f)(o);
  case FIOBJ_T_FLOAT:
    return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(o);
  case FIOBJ_T_STRING:
    tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), info)(o);
    if (!tmp.len)
      return 0;
    return (double)fio_atof(&tmp.buf);
  case FIOBJ_T_ARRAY:
    return (double)FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
  case FIOBJ_T_HASH:
    return (double)FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->to_f(o);
  }
  if (!o)
    return 0.0;
  return -1.0;
}

/* *****************************************************************************
FIOBJ Integers
***************************************************************************** */

#define FIO_REF_NAME fiobj___bignum
#define FIO_REF_TYPE intptr_t
#define FIO_REF_METADATA const FIOBJ_class_vtable_s *
#define FIO_REF_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___NUMBER_CLASS_VTBL;                                            \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REF_METADATA_DESTROY(m)                                            \
  do {                                                                         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_PTR_TAG(p) ((uintptr_t)p | FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p) FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE FIOBJ
#include __FILE__

#define FIO_NUMBER_ENCODE(i) (((uintptr_t)(i) << 3) | FIOBJ_T_NUMBER)
#define FIO_NUMBER_REVESE(i)                                                   \
  ((intptr_t)(((uintptr_t)(i) >> 3) |                                          \
              ((((uintptr_t)(i) >> ((sizeof(uintptr_t) * 8) - 1)) *            \
                ((uintptr_t)3 << ((sizeof(uintptr_t) * 8) - 3))))))

/** Creates a new Number object. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER),
                         new)(intptr_t i) {
  FIOBJ o = (FIOBJ)FIO_NUMBER_ENCODE(i);
  if (FIO_NUMBER_REVESE(o) == i)
    return o;
  o = fiobj___bignum_new2();
  FIO_PTR_MATH_RMASK(intptr_t, o, 3)[0] = i;
  return o;
}

/** Reads the number from a FIOBJ number. */
FIO_IFUNC intptr_t FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_NUMBER)
    return FIO_NUMBER_REVESE(i);
  return FIO_PTR_MATH_RMASK(intptr_t, i, 3)[0];
}

/** Reads the number from a FIOBJ number, fitting it in a double. */
FIO_IFUNC double FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), f)(FIOBJ i) {
  return (double)FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(i);
}

/** Frees a FIOBJ number. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), free)(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_NUMBER)
    return;
  fiobj___bignum_free2(i);
  return;
}
#undef FIO_NUMBER_ENCODE
#undef FIO_NUMBER_REVESE

/* *****************************************************************************
FIOBJ Floats
***************************************************************************** */

#define FIO_REF_NAME fiobj___bigfloat
#define FIO_REF_TYPE double
#define FIO_REF_METADATA const FIOBJ_class_vtable_s *
#define FIO_REF_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___FLOAT_CLASS_VTBL;                                             \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REF_METADATA_DESTROY(m)                                            \
  do {                                                                         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_PTR_TAG(p) ((uintptr_t)p | FIOBJ_T_OTHER)
#define FIO_PTR_UNTAG(p) FIOBJ_PTR_UNTAG(p)
#define FIO_PTR_TAG_TYPE FIOBJ
#include __FILE__

/** Creates a new Float object. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(double i) {
  FIOBJ ui;
  if (sizeof(double) <= sizeof(FIOBJ)) {
    union {
      double d;
      uintptr_t i;
    } punned;
    punned.i = 0; /* dead code, but leave it, just in case */
    punned.d = i;
    if ((punned.i & 7) == 0) {
      return (FIOBJ)(punned.i | FIOBJ_T_FLOAT);
    }
  }
  ui = fiobj___bigfloat_new2();
  FIO_PTR_MATH_RMASK(double, ui, 3)[0] = i;
  return ui;
}

/** Reads the integer part from a FIOBJ Float. */
FIO_IFUNC intptr_t FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), i)(FIOBJ i) {
  return (intptr_t)floor(FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(i));
}

/** Reads the number from a FIOBJ number, fitting it in a double. */
FIO_IFUNC double FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(FIOBJ i) {
  if (sizeof(double) <= sizeof(FIOBJ) && FIOBJ_TYPE_CLASS(i) == FIOBJ_T_FLOAT) {
    union {
      double d;
      uintptr_t i;
    } punned;
    punned.d = 0; /* dead code, but leave it, just in case */
    punned.i = (uintptr_t)i;
    punned.i = ((uintptr_t)i & (~(uintptr_t)7ULL));
    return punned.d;
  }
  return FIO_PTR_MATH_RMASK(double, i, 3)[0];
}

/** Frees a FIOBJ number. */
FIO_IFUNC void FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), free)(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_FLOAT)
    return;
  fiobj___bigfloat_free2(i);
  return;
}

/* *****************************************************************************
FIOBJ Basic Iteration
***************************************************************************** */

/**
 * Performs a task for each element held by the FIOBJ object.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the "stop" position - the number of elements processed + `start_at`.
 */
FIO_SFUNC uint32_t fiobj_each1(FIOBJ o, int32_t start_at,
                               int (*task)(FIOBJ child, void *arg), void *arg) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fallthrough */
  case FIOBJ_T_NUMBER:    /* fallthrough */
  case FIOBJ_T_STRING:    /* fallthrough */
  case FIOBJ_T_FLOAT:
    return 0;
  case FIOBJ_T_ARRAY:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), each)(o, start_at,
                                                               task, arg);
  case FIOBJ_T_HASH:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), each)(o, start_at, task,
                                                              arg);
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->each1(o, start_at, task, arg);
  }
  return 0;
}

/* *****************************************************************************
FIOBJ Hash Maps
***************************************************************************** */

/** Calculates an object's hash value for a specific hash map object. */
FIO_IFUNC uint64_t FIO_NAME2(fiobj, hash)(FIOBJ target_hash, FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    return fio_risky_hash(&o, sizeof(o), (uint64_t)target_hash + (uintptr_t)o);
  case FIOBJ_T_NUMBER: {
    uintptr_t tmp = FIO_NAME2(fiobj, i)(o);
    return fio_risky_hash(&tmp, sizeof(tmp), (uint64_t)target_hash);
  }
  case FIOBJ_T_FLOAT: {
    double tmp = FIO_NAME2(fiobj, f)(o);
    return fio_risky_hash(&tmp, sizeof(tmp), (uint64_t)target_hash);
  }
  case FIOBJ_T_STRING: /* fallthrough */
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                    hash)(o, (uint64_t)target_hash);
  case FIOBJ_T_ARRAY: {
    uint64_t h = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
    size_t c = 0;
    h += fio_risky_hash(&h, sizeof(h), (uint64_t)target_hash + FIOBJ_T_ARRAY);
    FIO_ARRAY_EACH(((FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY),
                              s) *)((uintptr_t)o & (~(uintptr_t)7))),
                   pos) {
      h += FIO_NAME2(fiobj, hash)(target_hash + FIOBJ_T_ARRAY + (c++), *pos);
    }
    return h;
  }
  case FIOBJ_T_HASH: {
    uint64_t h = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
    size_t c = 0;
    h += fio_risky_hash(&h, sizeof(h), (uint64_t)target_hash + FIOBJ_T_HASH);
    FIO_MAP_EACH2(FIO_NAME(fiobj, FIOBJ___NAME_HASH), o, pos) {
      h += FIO_NAME2(fiobj, hash)(target_hash + FIOBJ_T_HASH + (c++),
                                  pos->obj.key);
      h += FIO_NAME2(fiobj, hash)(target_hash + FIOBJ_T_HASH + (c++),
                                  pos->obj.value);
    }
    return h;
  }
  case FIOBJ_T_OTHER: {
    /* TODO: can we avoid "stringifying" the object? */
    fio_str_info_s tmp = (*fiobj_object_metadata(o))->to_s(o);
    return fio_risky_hash(tmp.buf, tmp.len, (uint64_t)target_hash);
  }
  }
  return 0;
}

/** Inserts a value to a hash map, with a default hash value calculation. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         set2)(FIOBJ hash, FIOBJ key, FIOBJ value) {
  return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set)(
      hash, FIO_NAME2(fiobj, hash)(hash, key), key, value, NULL);
}

/** Finds a value in a hash map, automatically calculating the hash value. */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), get2)(FIOBJ hash,
                                                                   FIOBJ key) {
  if (FIOBJ_TYPE_CLASS(hash) != FIOBJ_T_HASH)
    return FIOBJ_INVALID;
  return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                  get)(hash, FIO_NAME2(fiobj, hash)(hash, key), key);
}

/** Removes a value from a hash map, with a default hash value calculation. */
FIO_IFUNC int FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                       remove2)(FIOBJ hash, FIOBJ key, FIOBJ *old) {
  return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                  remove)(hash, FIO_NAME2(fiobj, hash)(hash, key), key, old);
}

/**
 * Sets a String value in a hash map, allocating the String and automatically
 * calculating the hash value.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         set3)(FIOBJ hash, const char *key, size_t len,
                               FIOBJ value) {
  FIOBJ tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(tmp, (char *)key, len);
  FIOBJ v = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set)(
      hash, fio_risky_hash(key, len, (uint64_t)hash), tmp, value, NULL);
  fiobj_free(tmp);
  return v;
}

/**
 * Finds a String value in a hash map, using a temporary String and
 * automatically calculating the hash value.
 */
FIO_IFUNC FIOBJ FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                         get3)(FIOBJ hash, const char *buf, size_t len) {
  if (FIOBJ_TYPE_CLASS(hash) != FIOBJ_T_HASH)
    return FIOBJ_INVALID;
  FIOBJ_STR_TEMP_VAR_STATIC(tmp, buf, len);
  FIOBJ v = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                     get)(hash, fio_risky_hash(buf, len, (uint64_t)hash), tmp);
  return v;
}

/**
 * Removes a String value in a hash map, using a temporary String and
 * automatically calculating the hash value.
 */
FIO_IFUNC int FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                       remove3)(FIOBJ hash, const char *buf, size_t len,
                                FIOBJ *old) {
  FIOBJ_STR_TEMP_VAR_STATIC(tmp, buf, len);
  int r = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), remove)(
      hash, fio_risky_hash(buf, len, (uint64_t)hash), tmp, old);
  FIOBJ_STR_TEMP_DESTROY(tmp);
  return r;
}

/* *****************************************************************************
FIOBJ JSON support (inline functions)
***************************************************************************** */

typedef struct {
  FIOBJ json;
  size_t level;
  uint8_t beautify;
} fiobj___json_format_internal__s;

/* internal helper funnction for recursive JSON formatting. */
FIOBJ_FUNC void
fiobj___json_format_internal__(fiobj___json_format_internal__s *, FIOBJ);

/** Helper function, calls `fiobj_hash_update_json` with string information */
FIO_IFUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                          update_json2)(FIOBJ hash, char *ptr, size_t len) {
  return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                  update_json)(hash, (fio_str_info_s){.buf = ptr, .len = len});
}

/**
 * Returns a JSON valid FIOBJ String, representing the object.
 *
 * If `dest` is an existing String, the formatted JSON data will be appended to
 * the existing string.
 */
FIO_IFUNC FIOBJ FIO_NAME2(fiobj, json)(FIOBJ dest, FIOBJ o, uint8_t beautify) {
  fiobj___json_format_internal__s args =
      (fiobj___json_format_internal__s){.json = dest, .beautify = beautify};
  if (FIOBJ_TYPE_CLASS(dest) != FIOBJ_T_STRING)
    args.json = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  fiobj___json_format_internal__(&args, o);
  return args.json;
}

/* *****************************************************************************
FIOBJ - Implementation
***************************************************************************** */
#ifdef FIOBJ_EXTERN_COMPLETE

/* *****************************************************************************
FIOBJ Basic Object vtable
***************************************************************************** */

FIOBJ_EXTERN_OBJ_IMP const FIOBJ_class_vtable_s FIOBJ___OBJECT_CLASS_VTBL = {
    .type_id = 99, /* type IDs below 100 are reserved. */
};

/* *****************************************************************************
FIOBJ Complex Iteration
***************************************************************************** */
typedef struct {
  FIOBJ obj;
  size_t pos;
} fiobj____stack_element_s;

#define FIO_ARRAY_NAME fiobj____active_stack
#define FIO_ARRAY_TYPE fiobj____stack_element_s
#define FIO_ARRAY_COPY(dest, src)                                              \
  do {                                                                         \
    (dest).obj = fiobj_dup((src).obj);                                         \
    (dest).pos = (src).pos;                                                    \
  } while (0)
#define FIO_ARRAY_TYPE_CMP(a, b) (a).obj == (b).obj
#define FIO_ARRAY_DESTROY(o) fiobj_free(o)
#include __FILE__
#define FIO_ARRAY_TYPE_CMP(a, b) (a).obj == (b).obj
#define FIO_ARRAY_NAME fiobj____stack
#define FIO_ARRAY_TYPE fiobj____stack_element_s
#include __FILE__

typedef struct {
  int (*task)(FIOBJ, void *);
  void *arg;
  FIOBJ next;
  size_t count;
  fiobj____stack_s stack;
  uint32_t end;
  uint8_t stop;
} fiobj_____each2_data_s;

FIO_SFUNC uint32_t fiobj____each2_element_count(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fallthrough */
  case FIOBJ_T_NUMBER:    /* fallthrough */
  case FIOBJ_T_STRING:    /* fallthrough */
  case FIOBJ_T_FLOAT:
    return 0;
  case FIOBJ_T_ARRAY:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
  case FIOBJ_T_HASH:
    return FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
  case FIOBJ_T_OTHER: /* fallthrough */
    return (*fiobj_object_metadata(o))->count(o);
  }
  return 0;
}
FIO_SFUNC int fiobj____each2_wrapper_task(FIOBJ child, void *arg) {
  fiobj_____each2_data_s *d = (fiobj_____each2_data_s *)arg;
  d->stop = (d->task(child, d->arg) == -1);
  ++d->count;
  if (d->stop)
    return -1;
  uint32_t c = fiobj____each2_element_count(child);
  if (c) {
    d->next = child;
    d->end = c;
    return -1;
  }
  return 0;
}

/**
 * Performs a task for the object itself and each element held by the FIOBJ
 * object or any of it's elements (a deep task).
 *
 * The order of performance is by order of appearance, as if all nesting levels
 * were flattened.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the number of elements processed.
 */
FIOBJ_FUNC uint32_t fiobj_each2(FIOBJ o, int (*task)(FIOBJ child, void *arg),
                                void *arg) {
  /* TODO - move to recursion with nesting limiter? */
  fiobj_____each2_data_s d = {
      .task = task,
      .arg = arg,
      .next = FIOBJ_INVALID,
      .stack = FIO_ARRAY_INIT,
  };
  fiobj____stack_element_s i = {.obj = o, .pos = 0};
  uint32_t end = fiobj____each2_element_count(o);
  fiobj____each2_wrapper_task(i.obj, &d);
  while (!d.stop && i.obj && i.pos < end) {
    i.pos = fiobj_each1(i.obj, i.pos, fiobj____each2_wrapper_task, &d);
    if (d.next != FIOBJ_INVALID) {
      if (fiobj____stack_count(&d.stack) + 1 > FIOBJ_MAX_NESTING) {
        FIO_LOG_ERROR("FIOBJ nesting level too deep (%u)."
                      "`fiobj_each2` stopping loop early.",
                      (unsigned int)fiobj____stack_count(&d.stack));
        d.stop = 1;
        continue;
      }
      fiobj____stack_push(&d.stack, i);
      i.pos = 0;
      i.obj = d.next;
      d.next = FIOBJ_INVALID;
      end = d.end;
    } else {
      /* re-collect end position to acommodate for changes */
      end = fiobj____each2_element_count(i.obj);
    }
    while (i.pos >= end && fiobj____stack_count(&d.stack)) {
      fiobj____stack_pop(&d.stack, &i);
      end = fiobj____each2_element_count(i.obj);
    }
  };
  fiobj____stack_destroy(&d.stack);
  return d.count;
}

/* *****************************************************************************
FIOBJ Hash / Array / Other (enumerable) Equality test.
***************************************************************************** */

FIO_SFUNC __thread size_t fiobj___test_eq_nested_level = 0;
/** Internal: compares two nestable objects. */
FIOBJ_FUNC unsigned char fiobj___test_eq_nested(FIOBJ restrict a,
                                                FIOBJ restrict b) {
  if (a == b)
    return 1;
  if (FIOBJ_TYPE_CLASS(a) != FIOBJ_TYPE_CLASS(b))
    return 0;
  if (fiobj____each2_element_count(a) != fiobj____each2_element_count(b))
    return 0;
  if (!fiobj____each2_element_count(a))
    return 1;
  if (fiobj___test_eq_nested_level >= FIOBJ_MAX_NESTING)
    return 0;
  ++fiobj___test_eq_nested_level;

  switch (FIOBJ_TYPE_CLASS(a)) {
  case FIOBJ_T_PRIMITIVE:
  case FIOBJ_T_NUMBER: /* fallthrough */
  case FIOBJ_T_FLOAT:  /* fallthrough */
  case FIOBJ_T_STRING: /* fallthrough */
    /* should never happen... this function is for enumerable objects */
    return a == b;
  case FIOBJ_T_ARRAY:
    /* test each array member with matching index */
    {
      const size_t count = fiobj____each2_element_count(a);
      for (size_t i = 0; i < count; ++i) {
        if (!FIO_NAME_BL(fiobj, eq)(
                FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(a, i),
                FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(b, i)))
          goto unequal;
      }
    }
    goto equal;
  case FIOBJ_T_HASH:
    FIO_MAP_EACH2(FIO_NAME(fiobj, FIOBJ___NAME_HASH), a, pos) {
      FIOBJ val = fiobj_hash_get2(b, pos->obj.key);
      if (!FIO_NAME_BL(fiobj, eq)(val, pos->obj.value))
        goto equal;
    }
    goto equal;
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(a))->is_eq(a, b);
  }
equal:
  --fiobj___test_eq_nested_level;
  return 1;
unequal:
  --fiobj___test_eq_nested_level;
  return 0;
}

/* *****************************************************************************
FIOBJ general helpers
***************************************************************************** */
FIO_SFUNC __thread char fiobj___tmp_buffer[256];

FIO_SFUNC uint32_t fiobj___count_noop(FIOBJ o) {
  return 0;
  (void)o;
}

/* *****************************************************************************
FIOBJ Integers (bigger numbers)
***************************************************************************** */

FIOBJ_FUNC unsigned char FIO_NAME_BL(fiobj___num, eq)(FIOBJ restrict a,
                                                      FIOBJ restrict b) {
  return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(a) ==
         FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(b);
}

FIOBJ_FUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER),
                                    cstr)(FIOBJ i) {
  size_t len =
      fio_ltoa(fiobj___tmp_buffer,
               FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i)(i), 10);
  fiobj___tmp_buffer[len] = 0;
  return (fio_str_info_s){.buf = fiobj___tmp_buffer, .len = len};
}

FIOBJ_EXTERN_OBJ_IMP const FIOBJ_class_vtable_s FIOBJ___NUMBER_CLASS_VTBL = {
    /**
     * MUST return a unique number to identify object type.
     *
     * Numbers (IDs) under 100 are reserved.
     */
    .type_id = FIOBJ_T_NUMBER,
    /** Test for equality between two objects with the same `type_id` */
    .is_eq = FIO_NAME_BL(fiobj___num, eq),
    /** Converts an object to a String */
    .to_s = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), cstr),
    /** Converts and object to an integer */
    .to_i = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), i),
    /** Converts and object to a float */
    .to_f = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), f),
    /** Returns the number of exposed elements held by the object, if any. */
    .count = fiobj___count_noop,
    /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
    .each1 = NULL,
    /** Deallocates the element (but NOT any of it's exposed elements). */
    .free2 = fiobj___bignum_free2,
};

/* *****************************************************************************
FIOBJ Floats (bigger / smaller doubles)
***************************************************************************** */

FIOBJ_FUNC unsigned char FIO_NAME_BL(fiobj___float, eq)(FIOBJ restrict a,
                                                        FIOBJ restrict b) {
  return FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), i)(a) ==
         FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), i)(b);
}

FIOBJ_FUNC fio_str_info_s FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT),
                                    cstr)(FIOBJ i) {
  size_t len =
      fio_ftoa(fiobj___tmp_buffer,
               FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f)(i), 10);
  fiobj___tmp_buffer[len] = 0;
  return (fio_str_info_s){.buf = fiobj___tmp_buffer, .len = len};
}

FIOBJ_EXTERN_OBJ_IMP const FIOBJ_class_vtable_s FIOBJ___FLOAT_CLASS_VTBL = {
    /**
     * MUST return a unique number to identify object type.
     *
     * Numbers (IDs) under 100 are reserved.
     */
    .type_id = FIOBJ_T_FLOAT,
    /** Test for equality between two objects with the same `type_id` */
    .is_eq = FIO_NAME_BL(fiobj___float, eq),
    /** Converts an object to a String */
    .to_s = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), cstr),
    /** Converts and object to an integer */
    .to_i = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), i),
    /** Converts and object to a float */
    .to_f = FIO_NAME2(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), f),
    /** Returns the number of exposed elements held by the object, if any. */
    .count = fiobj___count_noop,
    /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
    .each1 = NULL,
    /** Deallocates the element (but NOT any of it's exposed elements). */
    .free2 = fiobj___bigfloat_free2,
};

/* *****************************************************************************
FIOBJ JSON support - output
***************************************************************************** */

FIO_IFUNC void fiobj___json_format_internal_beauty_pad(FIOBJ json,
                                                       size_t level) {
  size_t pos = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), len)(json);
  fio_str_info_s tmp = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING),
                                resize)(json, (level << 1) + pos + 2);
  tmp.buf[pos++] = '\r';
  tmp.buf[pos++] = '\n';
  for (size_t i = 0; i < level; ++i) {
    tmp.buf[pos++] = ' ';
    tmp.buf[pos++] = ' ';
  }
}

FIOBJ_FUNC void
fiobj___json_format_internal__(fiobj___json_format_internal__s *args, FIOBJ o) {
  switch (FIOBJ_TYPE(o)) {
  case FIOBJ_T_TRUE:   /* fallthrough */
  case FIOBJ_T_FALSE:  /* fallthrough */
  case FIOBJ_T_NULL:   /* fallthrough */
  case FIOBJ_T_NUMBER: /* fallthrough */
  case FIOBJ_T_FLOAT:  /* fallthrough */
  {
    fio_str_info_s info = FIO_NAME2(fiobj, cstr)(o);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
    (args->json, info.buf, info.len);
    return;
  }
  case FIOBJ_T_STRING: /* fallthrough */
  default: {
    fio_str_info_s info = FIO_NAME2(fiobj, cstr)(o);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(args->json, "\"", 1);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_escape)
    (args->json, info.buf, info.len);
    FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(args->json, "\"", 1);
    return;
  }
  case FIOBJ_T_ARRAY:
    if (args->level == FIOBJ_MAX_NESTING) {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
      (args->json, "[ ]", 3);
      return;
    }
    {
      ++args->level;
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(args->json, "[", 1);
      const uint32_t len =
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), count)(o);
      for (size_t i = 0; i < len; ++i) {
        if (args->beautify) {
          fiobj___json_format_internal_beauty_pad(args->json, args->level);
        }
        FIOBJ child = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), get)(o, i);
        fiobj___json_format_internal__(args, child);
        if (i + 1 < len)
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
        (args->json, ",", 1);
      }
      --args->level;
      if (args->beautify) {
        fiobj___json_format_internal_beauty_pad(args->json, args->level);
      }
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(args->json, "]", 1);
    }
    return;
  case FIOBJ_T_HASH:
    if (args->level == FIOBJ_MAX_NESTING) {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
      (args->json, "{ }", 3);
      return;
    }
    {
      ++args->level;
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(args->json, "{", 1);
      size_t i = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), count)(o);
      if (i) {
        FIO_MAP_EACH2(FIO_NAME(fiobj, FIOBJ___NAME_HASH), o, couplet) {
          if (args->beautify) {
            fiobj___json_format_internal_beauty_pad(args->json, args->level);
          }
          fio_str_info_s info = FIO_NAME2(fiobj, cstr)(couplet->obj.key);
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
          (args->json, "\"", 1);
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_escape)
          (args->json, info.buf, info.len);
          FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
          (args->json, "\":", 2);
          fiobj___json_format_internal__(args, couplet->obj.value);
          if (--i)
            FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)
          (args->json, ",", 1);
        }
      }
      --args->level;
      if (args->beautify) {
        fiobj___json_format_internal_beauty_pad(args->json, args->level);
      }
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write)(args->json, "}", 1);
    }
    return;
  }
}

/* *****************************************************************************
FIOBJ JSON parsing
***************************************************************************** */

#define FIO_JSON
#include __FILE__

/* FIOBJ JSON parser */
typedef struct {
  fio_json_parser_s p;
  FIOBJ key;
  FIOBJ top;
  FIOBJ target;
  FIOBJ stack[JSON_MAX_DEPTH + 1];
  uint8_t so; /* stack offset */
} fiobj_json_parser_s;

static inline void fiobj_json_add2parser(fiobj_json_parser_s *p, FIOBJ o) {
  if (p->top) {
    if (FIOBJ_TYPE_CLASS(p->top) == FIOBJ_T_HASH) {
      if (p->key) {
        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), set2)(p->top, p->key, o);
        fiobj_free(p->key);
        p->key = FIOBJ_INVALID;
      } else {
        p->key = o;
      }
    } else {
      FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), push)(p->top, o);
    }
  } else {
    p->top = o;
  }
}

/** a NULL object was detected */
static inline void fio_json_on_null(fio_json_parser_s *p) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p,
                        FIO_NAME(fiobj, FIOBJ___NAME_NULL)());
}
/** a TRUE object was detected */
static inline void fio_json_on_true(fio_json_parser_s *p) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p,
                        FIO_NAME(fiobj, FIOBJ___NAME_TRUE)());
}
/** a FALSE object was detected */
static inline void fio_json_on_false(fio_json_parser_s *p) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p,
                        FIO_NAME(fiobj, FIOBJ___NAME_FALSE)());
}
/** a Numberl was detected (long long). */
static inline void fio_json_on_number(fio_json_parser_s *p, long long i) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p,
                        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_NUMBER), new)(i));
}
/** a Float was detected (double). */
static inline void fio_json_on_float(fio_json_parser_s *p, double f) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p,
                        FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_FLOAT), new)(f));
}
/** a String was detected (int / float). update `pos` to point at ending */
static inline void fio_json_on_string(fio_json_parser_s *p, const void *start,
                                      size_t len) {
  FIOBJ str = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), new)();
  FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_STRING), write_unescape)
  (str, start, len);
  fiobj_json_add2parser((fiobj_json_parser_s *)p, str);
}
/** a dictionary object was detected */
static inline int fio_json_on_start_object(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  if (pr->target) {
    /* push NULL, don't free the objects */
    pr->stack[pr->so++] = FIOBJ_INVALID;
    pr->top = pr->target;
    pr->target = FIOBJ_INVALID;
  } else {
    FIOBJ hash = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH), new)();
    fiobj_json_add2parser(pr, hash);
    pr->stack[pr->so++] = pr->top;
    pr->top = hash;
  }
  return 0;
}
/** a dictionary object closure detected */
static inline void fio_json_on_end_object(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  if (pr->key) {
    FIO_LOG_WARNING("(JSON parsing) malformed JSON, "
                    "ignoring dangling Hash key.");
    fiobj_free(pr->key);
    pr->key = FIOBJ_INVALID;
  }
  pr->top = FIOBJ_INVALID;
  if (pr->so)
    pr->top = pr->stack[--pr->so];
}
/** an array object was detected */
static int fio_json_on_start_array(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  if (pr->target)
    return -1;
  FIOBJ ary = FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_ARRAY), new)();
  fiobj_json_add2parser(pr, ary);
  pr->stack[pr->so++] = pr->top;
  pr->top = ary;
  return 0;
}
/** an array closure was detected */
static inline void fio_json_on_end_array(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  pr->top = FIOBJ_INVALID;
  if (pr->so)
    pr->top = pr->stack[--pr->so];
}
/** the JSON parsing is complete */
static void fio_json_on_json(fio_json_parser_s *p) {
  (void)p; /* nothing special... right? */
}
/** the JSON parsing is complete */
static inline void fio_json_on_error(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  fiobj_free(pr->stack[0]);
  fiobj_free(pr->key);
  *pr = (fiobj_json_parser_s){.top = FIOBJ_INVALID};
  FIO_LOG_DEBUG("JSON on_error callback called.");
}

/**
 * Updates a Hash using JSON data.
 *
 * Parsing errors and non-dictionary object JSON data are silently ignored,
 * attempting to update the Hash as much as possible before any errors
 * encountered.
 *
 * Conflicting Hash data is overwritten (preferring the new over the old).
 *
 * Returns the number of bytes consumed. On Error, 0 is returned and no data is
 * consumed.
 */
FIOBJ_FUNC size_t FIO_NAME(FIO_NAME(fiobj, FIOBJ___NAME_HASH),
                           update_json)(FIOBJ hash, fio_str_info_s str) {
  if (hash == FIOBJ_INVALID)
    return 0;
  fiobj_json_parser_s p = {.top = FIOBJ_INVALID, .target = hash};
  size_t consumed = fio_json_parse(&p.p, str.buf, str.len);
  fiobj_free(p.key);
  if (p.top != hash)
    fiobj_free(p.top);
  return consumed;
}

/** Returns a JSON valid FIOBJ String, representing the object. */
FIOBJ_FUNC FIOBJ fiobj_json_parse(fio_str_info_s str, size_t *consumed_p) {
  fiobj_json_parser_s p = {.top = FIOBJ_INVALID};
  register const size_t consumed = fio_json_parse(&p.p, str.buf, str.len);
  if (consumed_p) {
    *consumed_p = consumed;
  }
  if (!consumed || p.p.depth) {
    if (p.top) {
      FIO_LOG_DEBUG("WARNING - JSON failed secondary validation, no on_error");
    }
#if DEBUG
    FIOBJ s = FIO_NAME2(fiobj, json)(FIOBJ_INVALID, p.top, 0);
    FIO_LOG_DEBUG("JSON data being deleted:\n%s",
                  FIO_NAME2(fiobj, cstr)(s).buf);
    fiobj_free(s);
#endif
    fiobj_free(p.stack[0]);
    p.top = FIOBJ_INVALID;
  }
  fiobj_free(p.key);
  return p.top;
}

/* *****************************************************************************
FIOBJ cleanup
***************************************************************************** */

#endif /* FIOBJ_EXTERN_COMPLETE */
#undef FIOBJ_FUNC
#undef FIOBJ_IFUNC
#undef FIOBJ_EXTERN
#undef FIOBJ_EXTERN_COMPLETE
#undef FIOBJ_EXTERN_OBJ
#undef FIOBJ_EXTERN_OBJ_IMP
#endif /* FIO_FIOBJ */
#undef FIO_FIOBJ