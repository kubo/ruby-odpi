/*
 * rbdpi-rowid.c -- part of ruby-odpi
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

static VALUE cRowid;

static void rowid_free(void *arg)
{
    dpiRowid_release(((rowid_t*)arg)->handle);
    xfree(arg);
}

static const struct rb_data_type_struct rowid_data_type = {
    "ODPI::Dpi::Rowid",
    {NULL, rowid_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE rowid_alloc(VALUE klass)
{
    rowid_t *rowid;

    return TypedData_Make_Struct(klass, rowid_t, &rowid_data_type, rowid);
}

static VALUE rowid_initialize_copy(VALUE self, VALUE other)
{
    rowid_t *rowid = rbdpi_to_rowid(self);

    *rowid = *rbdpi_to_rowid(other);
    if (rowid->handle != NULL) {
        CHK(dpiRowid_addRef(rowid->handle));
    }
    return self;
}

static VALUE rowid_to_s(VALUE self)
{
    rowid_t *rowid = rbdpi_to_rowid(self);
    const char *val;
    uint32_t len;

    CHK(dpiRowid_getStringValue(rowid->handle, &val, &len));
    return rb_usascii_str_new(val, len);
}

void Init_rbdpi_rowid(VALUE mDpi)
{
    cRowid = rb_define_class_under(mDpi, "Rowid", rb_cObject);
    rb_define_alloc_func(cRowid, rowid_alloc);
    rb_define_method(cRowid, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cRowid, "initialize_copy", rowid_initialize_copy, 1);
    rb_define_method(cRowid, "to_s", rowid_to_s, 0);
}

VALUE rbdpi_from_rowid(dpiRowid *handle)
{
    rowid_t *rowid;
    VALUE obj = TypedData_Make_Struct(cRowid, rowid_t, &rowid_data_type, rowid);

    rowid->handle = handle;
    return obj;
}

rowid_t *rbdpi_to_rowid(VALUE obj)
{
    rowid_t *rowid = (rowid_t *)rb_check_typeddata(obj, &rowid_data_type);

    if (rowid->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return rowid;
}
