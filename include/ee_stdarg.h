#ifndef EE_STDARG_H
#define EE_STDARG_H

/*
 * va_list and va_start as MWCC compiles them for the EE. The register save
 * area ends at the incoming stack arguments; __builtin_args_info(2) is the
 * number of named arguments in registers. The game never calls va_end (a
 * no-op on the EE). A host build (MH2_NATIVE) takes its compiler's builtins.
 */
#ifndef MH2_NATIVE
typedef char *va_list;
#define va_start(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (__builtin_args_info(2) >= 8 ? 0 : (8 - __builtin_args_info(2)) * 8))
#else
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#endif

#endif /* EE_STDARG_H */
