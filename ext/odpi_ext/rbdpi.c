/*
 * rbdpi.c -- part of ruby-odpi
 *
 * URL: https://github.com/kubo/ruby-odpi
 *
 * ------------------------------------------------------
 *
 * Copyright 2017 Kubo Takehiro <kubo@jiubao.org>
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of the authors.
 *
 */
#include "rbdpi.h"

const dpiContext *rbdpi_g_context;

VALUE rbdpi_initialize_error(VALUE self)
{
    rb_raise(rb_eRuntimeError, "could not initialize by %s::new", rb_obj_classname(self));
    return Qnil;
}

/* ODPI::oracle_client_version */
static VALUE oracle_client_version(VALUE klass)
{
    dpiVersionInfo ver;

    CHK(dpiContext_getClientVersion(rbdpi_g_context, &ver));
    return rbdpi_from_dpiVersionInfo(&ver);
}

void
Init_odpi_ext()
{
    VALUE mODPI = rb_define_module("ODPI");
    VALUE mDpi = rb_define_module_under(mODPI, "Dpi");
    dpiErrorInfo error;
    dpiContext *context;

    if (dpiContext_create(DPI_MAJOR_VERSION, DPI_MINOR_VERSION, &context, &error) != DPI_SUCCESS) {
        rb_raise(rb_eLoadError, "%.*s", error.messageLength, error.message);
    }
    rbdpi_g_context = context;

    rb_define_const(mDpi, "ODPI_C_VERSION", rb_usascii_str_new_cstr(DPI_VERSION_STRING));
    rb_define_singleton_method(mDpi, "oracle_client_version", oracle_client_version, 0);

    Init_rbdpi_conn(mDpi);
    Init_rbdpi_deq_options(mDpi);
    Init_rbdpi_enq_options(mDpi);
    Init_rbdpi_enum();
    Init_rbdpi_lob(mDpi);
    Init_rbdpi_msg_props(mDpi);
    Init_rbdpi_object(mDpi);
    Init_rbdpi_object_attr(mDpi);
    Init_rbdpi_object_type(mDpi);
    Init_rbdpi_pool(mDpi);
    Init_rbdpi_rowid(mDpi);
    Init_rbdpi_stmt(mDpi);
    Init_rbdpi_struct(mDpi);
    Init_rbdpi_subscr(mDpi);
    Init_rbdpi_var(mDpi);
    Init_rbdpi_version_info(mDpi);
}
