/*
 * Header file for common error description library.
 *
 * Copyright 1988, Student Information Processing Board of the
 * Massachusetts Institute of Technology.
 *
 * For copyright and distribution info, see the documentation supplied
 * with this package.
 */

#ifndef COM_ERR__H
#define COM_ERR__H

#include <stdarg.h>

typedef long errcode_t;
typedef void (*com_err_handler_t)(const char *, errcode_t, const char *, va_list);

struct error_table;

void com_err(const char *progname, errcode_t code, const char *fmt, ...);
void com_err_va(const char *progname, errcode_t code, const char *fmt,
		va_list args);
char const *error_message(errcode_t code);
com_err_handler_t set_com_err_hook(com_err_handler_t handler);
com_err_handler_t reset_com_err_hook(void);

int init_error_table(const char *const *msgs, int base, int count);

errcode_t add_error_table(const struct error_table *et);
errcode_t remove_error_table(const struct error_table *et);

#endif
