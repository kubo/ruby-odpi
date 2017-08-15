/*
 * rbdpi-object-type.c -- part of ruby-odpi
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

static VALUE cObjectType;

static void object_type_mark(void *arg)
{
    object_type_t *objtype = (object_type_t *)arg;

    rb_gc_mark(objtype->elem_datatype);
}

static void object_type_free(void *arg)
{
    dpiObjectType_release(((object_type_t*)arg)->handle);
    xfree(arg);
}

static const struct rb_data_type_struct object_type_data_type = {
    "ODPI::Dpi::ObjectType",
    {object_type_mark, object_type_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE object_type_alloc(VALUE klass)
{
    object_type_t *objtype;

    return TypedData_Make_Struct(klass, object_type_t, &object_type_data_type, objtype);
}

static VALUE object_type_initialize_copy(VALUE self, VALUE other)
{
    *rbdpi_to_object_type(self) = *rbdpi_to_object_type(other);
    return self;
}

static VALUE object_type_create_object(VALUE self)
{
    object_type_t *objtype = rbdpi_to_object_type(self);
    dpiObject *object;

    CHK(dpiObjectType_createObject(objtype->handle, &object));
    return rbdpi_from_object(object, self);
}

static VALUE object_type_get_schema(VALUE self)
{
    object_type_t *objtype = rbdpi_to_object_type(self);

    return rb_external_str_new_with_enc(objtype->info.schema, objtype->info.schemaLength, objtype->enc.enc);
}

static VALUE object_type_get_name(VALUE self)
{
    object_type_t *objtype = rbdpi_to_object_type(self);

    return rb_external_str_new_with_enc(objtype->info.name, objtype->info.nameLength, objtype->enc.enc);
}

static VALUE object_type_is_collection_p(VALUE self)
{
    object_type_t *objtype = rbdpi_to_object_type(self);

    return objtype->info.isCollection ? Qtrue : Qfalse;
}

static VALUE object_type_get_element_type_info(VALUE self)
{
    object_type_t *objtype = rbdpi_to_object_type(self);

    return objtype->elem_datatype;
}

static VALUE object_type_get_num_attributes(VALUE self)
{
    object_type_t *objtype = rbdpi_to_object_type(self);

    return INT2FIX(objtype->info.numAttributes);
}

static VALUE object_type_get_attributes(VALUE self)
{
    object_type_t *objtype = rbdpi_to_object_type(self);
    uint16_t idx, num_attrs;
    dpiObjectAttr **attrs;
    VALUE ary;

    num_attrs = objtype->info.numAttributes;

    if (num_attrs == 0) {
        return rb_ary_new();
    }
    attrs = ALLOCA_N(dpiObjectAttr *, num_attrs);
    CHK(dpiObjectType_getAttributes(objtype->handle, num_attrs, attrs));

    ary = rb_ary_new_capa(num_attrs);
    for (idx = 0; idx < num_attrs; idx++) {
        rb_ary_push(ary, rbdpi_from_object_attr(attrs[idx], &objtype->enc));
    }
    return ary;
}

void Init_rbdpi_object_type(VALUE mDpi)
{
    cObjectType = rb_define_class_under(mDpi, "ObjectType", rb_cObject);
    rb_define_alloc_func(cObjectType, object_type_alloc);
    rb_define_method(cObjectType, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cObjectType, "initialize_copy", object_type_initialize_copy, 1);
    rb_define_method(cObjectType, "create_object", object_type_create_object, 0);
    rb_define_method(cObjectType, "schema", object_type_get_schema, 0);
    rb_define_method(cObjectType, "name", object_type_get_name, 0);
    rb_define_method(cObjectType, "is_collection?", object_type_is_collection_p, 0);
    rb_define_method(cObjectType, "element_type_info", object_type_get_element_type_info, 0);
    rb_define_method(cObjectType, "num_attributes", object_type_get_num_attributes, 0);
    rb_define_method(cObjectType, "attributes", object_type_get_attributes, 0);
}

VALUE rbdpi_from_object_type(dpiObjectType *handle, const rbdpi_enc_t *enc)
{
    object_type_t *objtype;
    VALUE obj = TypedData_Make_Struct(cObjectType, object_type_t, &object_type_data_type, objtype);

    objtype->handle = handle;
    objtype->enc = *enc;
    CHK(dpiObjectType_getInfo(handle, &objtype->info));
    objtype->elem_datatype = rbdpi_from_dpiDataTypeInfo(&objtype->info.elementTypeInfo, obj, enc);
    return obj;
}

object_type_t *rbdpi_to_object_type(VALUE obj)
{
    object_type_t *object_type = (object_type_t *)rb_check_typeddata(obj, &object_type_data_type);

    if (object_type->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return object_type;
}
