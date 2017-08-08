/*
 * rbdpi-enq-options.c -- part of ruby-odpi
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

static VALUE cEnqOptions;

static void enq_options_free(void *arg)
{
    dpiEnqOptions_release(((enq_options_t*)arg)->handle);
    xfree(arg);
}

static const struct rb_data_type_struct enq_options_data_type = {
    "ODPI::Dpi::EnqOptions",
    {NULL, enq_options_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE enq_options_alloc(VALUE klass)
{
    enq_options_t *enq_options;

    return TypedData_Make_Struct(klass, enq_options_t, &enq_options_data_type, enq_options);
}

static VALUE enq_options_initialize_copy(VALUE self, VALUE other)
{
    enq_options_t *enq_options = rbdpi_to_enq_options(self);

    *enq_options = *rbdpi_to_enq_options(other);
    if (enq_options->handle != NULL) {
        CHK(dpiEnqOptions_addRef(enq_options->handle));
    }
    return self;
}

static VALUE enq_options_get_transformation(VALUE self)
{
    enq_options_t *options = rbdpi_to_enq_options(self);
    const char *val;
    uint32_t len;

    CHK(dpiEnqOptions_getTransformation(options->handle, &val, &len));
    return rb_external_str_new_with_enc(val, len, options->enc);
}

static VALUE enq_options_get_visibility(VALUE self)
{
    enq_options_t *options = rbdpi_to_enq_options(self);
    dpiVisibility val;

    CHK(dpiEnqOptions_getVisibility(options->handle, &val));
    return rbdpi_from_dpiVisibility(val);
}

static VALUE enq_options_set_delivery_mode(VALUE self, VALUE val)
{
    enq_options_t *options = rbdpi_to_enq_options(self);

    CHK(dpiEnqOptions_setDeliveryMode(options->handle, rbdpi_to_dpiMessageDeliveryMode(val)));
    return val;
}

static VALUE enq_options_set_transformation(VALUE self, VALUE val)
{
    enq_options_t *options = rbdpi_to_enq_options(self);

    CHK_STR_ENC(val, options->enc);
    CHK(dpiEnqOptions_setTransformation(options->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    return val;
}

static VALUE enq_options_set_visibility(VALUE self, VALUE val)
{
    enq_options_t *options = rbdpi_to_enq_options(self);

    CHK(dpiEnqOptions_setVisibility(options->handle, rbdpi_to_dpiVisibility(val)));
    return val;
}

void Init_rbdpi_enq_options(VALUE mDpi)
{
    cEnqOptions = rb_define_class_under(mDpi, "EnqOptions", rb_cObject);
    rb_define_alloc_func(cEnqOptions, enq_options_alloc);
    rb_define_method(cEnqOptions, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cEnqOptions, "initialize_copy", enq_options_initialize_copy, 1);
    rb_define_method(cEnqOptions, "transformation", enq_options_get_transformation, 0);
    rb_define_method(cEnqOptions, "visibility", enq_options_get_visibility, 0);
    rb_define_method(cEnqOptions, "delivery_mode=", enq_options_set_delivery_mode, 1);
    rb_define_method(cEnqOptions, "transformation=", enq_options_set_transformation, 1);
    rb_define_method(cEnqOptions, "visibility=", enq_options_set_visibility, 1);
}

VALUE rbdpi_from_enq_options(dpiEnqOptions *handle, rb_encoding *enc)
{
    enq_options_t *options;
    VALUE obj = TypedData_Make_Struct(cEnqOptions, enq_options_t, &enq_options_data_type, options);

    options->handle = handle;
    options->enc = enc;
    return obj;
}

enq_options_t *rbdpi_to_enq_options(VALUE obj)
{
    enq_options_t *enq_options = (enq_options_t *)rb_check_typeddata(obj, &enq_options_data_type);

    if (enq_options->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return enq_options;
}
