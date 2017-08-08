/*
 * rbdpi-object-attr.c -- part of ruby-odpi
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

static VALUE cObjectAttr;

static void object_attr_mark(void *arg)
{
    object_attr_t *objattr = (object_attr_t*)arg;

    rb_gc_mark(objattr->objtype);
}

static void object_attr_free(void *arg)
{
    dpiObjectAttr_release(((object_attr_t*)arg)->handle);
    xfree(arg);
}

static const struct rb_data_type_struct object_attr_data_type = {
    "ODPI::Dpi::ObjectAttr",
    {object_attr_mark, object_attr_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE object_attr_alloc(VALUE klass)
{
    object_attr_t *object_attr;

    return TypedData_Make_Struct(klass, object_attr_t, &object_attr_data_type, object_attr);
}

static VALUE object_attr_initialize_copy(VALUE self, VALUE other)
{
    object_attr_t *object_attr = rbdpi_to_object_attr(self);

    *object_attr = *rbdpi_to_object_attr(other);
    if (object_attr->handle != NULL) {
        CHK(dpiObjectAttr_addRef(object_attr->handle));
    }
    return self;
}

static VALUE object_attr_get_name(VALUE self)
{
    object_attr_t *objattr = rbdpi_to_object_attr(self);

    return rb_external_str_new_with_enc(objattr->info.name, objattr->info.nameLength, objattr->enc.enc);
}

static VALUE object_attr_get_oracle_type(VALUE self)
{
    object_attr_t *objattr = rbdpi_to_object_attr(self);

    return rbdpi_from_dpiOracleTypeNum(objattr->info.oracleTypeNum);
}

static VALUE object_attr_get_default_native_type(VALUE self)
{
    object_attr_t *objattr = rbdpi_to_object_attr(self);

    return rbdpi_from_dpiNativeTypeNum(objattr->info.defaultNativeTypeNum);
}

static VALUE object_attr_get_object_type(VALUE self)
{
    object_attr_t *objattr = rbdpi_to_object_attr(self);

    return objattr->objtype;
}

void Init_rbdpi_object_attr(VALUE mDpi)
{
    cObjectAttr = rb_define_class_under(mDpi, "ObjectAttr", rb_cObject);
    rb_define_alloc_func(cObjectAttr, object_attr_alloc);
    rb_define_method(cObjectAttr, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cObjectAttr, "initialize_copy", object_attr_initialize_copy, 1);
    rb_define_method(cObjectAttr, "name", object_attr_get_name, 0);
    rb_define_method(cObjectAttr, "oracle_type", object_attr_get_oracle_type, 0);
    rb_define_method(cObjectAttr, "default_native_type", object_attr_get_default_native_type, 0);
    rb_define_method(cObjectAttr, "object_type", object_attr_get_object_type, 0);
}

VALUE rbdpi_from_object_attr(dpiObjectAttr *handle, const rbdpi_enc_t *enc)
{
    object_attr_t *objattr;
    VALUE obj = TypedData_Make_Struct(cObjectAttr, object_attr_t, &object_attr_data_type, objattr);

    objattr->handle = handle;
    objattr->enc = *enc;
    CHK(dpiObjectAttr_getInfo(handle, &objattr->info));
    if (objattr->info.objectType) {
        objattr->objtype = rbdpi_from_object_type(objattr->info.objectType, &objattr->enc);
    } else {
        objattr->objtype = Qnil;
    }
    return obj;
}

object_attr_t *rbdpi_to_object_attr(VALUE obj)
{
    object_attr_t *object_attr = (object_attr_t *)rb_check_typeddata(obj, &object_attr_data_type);

    if (object_attr->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return object_attr;
}
