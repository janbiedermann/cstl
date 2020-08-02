/* *****************************************************************************










                            Common Cleanup










***************************************************************************** */

/* *****************************************************************************
Common cleanup
***************************************************************************** */
#ifndef FIO_STL_KEEP__

#undef FIO_EXTERN
#undef SFUNC
#undef IFUNC
#undef SFUNC_
#undef IFUNC_
#undef FIO_PTR_TAG
#undef FIO_PTR_UNTAG
#undef FIO_PTR_TAG_TYPE
#undef FIO_PTR_TAG_VALIDATE
#undef FIO_PTR_TAG_VALID_OR_RETURN
#undef FIO_PTR_TAG_VALID_OR_RETURN_VOID
#undef FIO_PTR_TAG_VALID_OR_GOTO
#undef FIO_MEM_CALLOC_
#undef FIO_MEM_REALLOC_
#undef FIO_MEM_FREE_
#undef FIO_MEM_INTERNAL_MALLOC_
#undef FIO_MALLOC_TMP_USE_SYSTEM

/* undefine FIO_EXTERN_COMPLETE only if it was defined locally */
#if FIO_EXTERN_COMPLETE == 2
#undef FIO_EXTERN_COMPLETE
#endif

#else

#undef SFUNC
#undef IFUNC
#define SFUNC SFUNC_
#define IFUNC IFUNC_

#endif /* !FIO_STL_KEEP__ */