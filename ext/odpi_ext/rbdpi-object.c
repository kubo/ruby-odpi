/*
 * rbdpi-object.c -- part of ruby-odpi
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

static VALUE cObject;

static void object_mark(void *arg)
{
    object_t *object = (object_t*)arg;

    rb_gc_mark(object->objtype);
}

static void object_free(void *arg)
{
    dpiObject_release(((object_t*)arg)->handle);
    xfree(arg);
}

static const struct rb_data_type_struct object_data_type = {
    "ODPI::Dpi::Object",
    {object_mark, object_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE object_alloc(VALUE klass)
{
    object_t *object;

    return TypedData_Make_Struct(klass, object_t, &object_data_type, object);
}

static VALUE object_initialize_copy(VALUE self, VALUE other)
{
    object_t *object = rbdpi_to_object(self);

    *object = *rbdpi_to_object(other);
    if (object->handle != NULL) {
        CHK(dpiObject_addRef(object->handle));
    }
    return self;
}

static VALUE object_append_element(VALUE self, VALUE val, VALUE native_type)
{
    object_t *object = rbdpi_to_object(self);
    object_type_t *objtype = object->object_type;
    dpiNativeTypeNum type = rbdpi_to_dpiNativeTypeNum(native_type);
    dpiData data;

    val = rbdpi_to_dpiData(&data, val, type, &objtype->enc, objtype->info.elementOracleTypeNum);
    CHK(dpiObject_appendElement(object->handle, type, &data));
    RB_GC_GUARD(val);
    return self;
}

static VALUE object_dup(VALUE self)
{
    object_t *object = rbdpi_to_object(self);
    dpiObject *handle;

    CHK(dpiObject_copy(object->handle, &handle));
    return rbdpi_from_object(handle, object->objtype);
}

static VALUE object_delete_element_by_index(VALUE self, VALUE idx)
{
    object_t *object = rbdpi_to_object(self);

    CHK(dpiObject_deleteElementByIndex(object->handle, NUM2INT(idx)));
    return self;
}

static VALUE object_get_attribute_value(VALUE self, VALUE attr, VALUE native_type)
{
    object_t *object = rbdpi_to_object(self);
    object_attr_t *objattr = rbdpi_to_object_attr(attr);
    dpiNativeTypeNum type = rbdpi_to_dpiNativeTypeNum(native_type);
    dpiData data;

    CHK(dpiObject_getAttributeValue(object->handle, objattr->handle, type, &data));
    return rbdpi_from_dpiData(&data, type, &objattr->enc, objattr->info.oracleTypeNum, objattr->objtype);
}

static VALUE object_get_element_exists_by_index(VALUE self, VALUE idx)
{
    object_t *object = rbdpi_to_object(self);
    int exists;

    CHK(dpiObject_getElementExistsByIndex(object->handle, NUM2INT(idx), &exists));
    return exists ? Qtrue : Qfalse;
}

static VALUE object_get_element_value_by_index(VALUE self, VALUE idx, VALUE native_type)
{
    object_t *object = rbdpi_to_object(self);
    object_type_t *objtype = object->object_type;
    dpiNativeTypeNum type = rbdpi_to_dpiNativeTypeNum(native_type);
    dpiData data;

    CHK(dpiObject_getElementValueByIndex(object->handle, NUM2INT(idx), type, &data));
    return rbdpi_from_dpiData(&data, type, &objtype->enc, objtype->info.elementOracleTypeNum, object->objtype);
}

static VALUE object_get_first_index(VALUE self)
{
    object_t *object = rbdpi_to_object(self);
    int32_t index;
    int exists;

    CHK(dpiObject_getFirstIndex(object->handle, &index, &exists));
    return exists ? INT2NUM(index) : Qnil;
}

static VALUE object_get_last_index(VALUE self)
{
    object_t *object = rbdpi_to_object(self);
    int32_t index;
    int exists;

    CHK(dpiObject_getLastIndex(object->handle, &index, &exists));
    return exists ? INT2NUM(index) : Qnil;
}

static VALUE object_get_next_index(VALUE self, VALUE idx)
{
    object_t *object = rbdpi_to_object(self);
    int32_t next_index;
    int exists;

    CHK(dpiObject_getNextIndex(object->handle, NUM2INT(idx), &next_index, &exists));
    return exists ? INT2NUM(next_index) : Qnil;
}

static VALUE object_get_prev_index(VALUE self, VALUE idx)
{
    object_t *object = rbdpi_to_object(self);
    int32_t prev_index;
    int exists;

    CHK(dpiObject_getPrevIndex(object->handle, NUM2INT(idx), &prev_index, &exists));
    return exists ? INT2NUM(prev_index) : Qnil;
}

static VALUE object_get_size(VALUE self)
{
    object_t *object = rbdpi_to_object(self);
    int32_t size;

    CHK(dpiObject_getSize(object->handle, &size));
    return INT2NUM(size);
}

static VALUE object_set_attribute_value(VALUE self, VALUE attr, VALUE native_type, VALUE val)
{
    object_t *object = rbdpi_to_object(self);
    object_attr_t *objattr = rbdpi_to_object_attr(attr);
    dpiNativeTypeNum type = rbdpi_to_dpiNativeTypeNum(native_type);
    dpiData data;

    val = rbdpi_to_dpiData(&data, val, type, &objattr->enc, objattr->info.oracleTypeNum);
    CHK(dpiObject_setAttributeValue(object->handle, objattr->handle, type, &data));
    RB_GC_GUARD(val);
    return self;
}

static VALUE object_set_element_value_by_index(VALUE self, VALUE idx, VALUE native_type, VALUE val)
{
    object_t *object = rbdpi_to_object(self);
    object_type_t *objtype = object->object_type;
    dpiNativeTypeNum type = rbdpi_to_dpiNativeTypeNum(native_type);
    dpiData data;

    val = rbdpi_to_dpiData(&data, val, type, &objtype->enc, objtype->info.elementOracleTypeNum);
    CHK(dpiObject_setElementValueByIndex(object->handle, NUM2INT(idx), type, &data));
    RB_GC_GUARD(val);
    return self;
}

static VALUE object_trim(VALUE self, VALUE size)
{
    object_t *object = rbdpi_to_object(self);

    CHK(dpiObject_trim(object->handle, NUM2UINT(size)));
    return self;
}

void Init_rbdpi_object(VALUE mDpi)
{
    cObject = rb_define_class_under(mDpi, "Object", rb_cObject);
    rb_define_alloc_func(cObject, object_alloc);
    rb_define_method(cObject, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cObject, "initialize_copy", object_initialize_copy, 1);
    rb_define_method(cObject, "append_element", object_append_element, 2);
    rb_define_method(cObject, "dup", object_dup, 0);
    rb_define_method(cObject, "delete_element_by_index", object_delete_element_by_index, 1);
    rb_define_method(cObject, "attribute_value", object_get_attribute_value, 2);
    rb_define_method(cObject, "element_exists_by_index?", object_get_element_exists_by_index, 1);
    rb_define_method(cObject, "element_value_by_index", object_get_element_value_by_index, 2);
    rb_define_method(cObject, "first_index", object_get_first_index, 0);
    rb_define_method(cObject, "last_index", object_get_last_index, 0);
    rb_define_method(cObject, "next_index", object_get_next_index, 1);
    rb_define_method(cObject, "prev_index", object_get_prev_index, 1);
    rb_define_method(cObject, "size", object_get_size, 0);
    rb_define_method(cObject, "set_attribute_value", object_set_attribute_value, 3);
    rb_define_method(cObject, "set_element_value_by_index", object_set_element_value_by_index, 3);
    rb_define_method(cObject, "trim", object_trim, 1);
}

VALUE rbdpi_from_object(dpiObject *handle, VALUE objtype)
{
    object_t *object;
    VALUE obj = TypedData_Make_Struct(cObject, object_t, &object_data_type, object);

    object->handle = handle;
    object->objtype = objtype;
    object->object_type = rbdpi_to_object_type(objtype);
    return obj;
}

object_t *rbdpi_to_object(VALUE obj)
{
    object_t *object = (object_t *)rb_check_typeddata(obj, &object_data_type);

    if (object->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return object;
}
