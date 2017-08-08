/*
 * rbdpi-deq-options.c -- part of ruby-odpi
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

static VALUE cDeqOptions;

static void deq_options_free(void *arg)
{
    dpiDeqOptions_release(((deq_options_t*)arg)->handle);
    xfree(arg);
}

static const struct rb_data_type_struct deq_options_data_type = {
    "ODPI::Dpi::DeqOptions",
    {NULL, deq_options_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE deq_options_alloc(VALUE klass)
{
    deq_options_t *deq_options;

    return TypedData_Make_Struct(klass, deq_options_t, &deq_options_data_type, deq_options);
}

static VALUE deq_options_initialize_copy(VALUE self, VALUE other)
{
    deq_options_t *deq_options = rbdpi_to_deq_options(self);

    *deq_options = *rbdpi_to_deq_options(other);
    if (deq_options->handle != NULL) {
        CHK(dpiDeqOptions_addRef(deq_options->handle));
    }
    return self;
}

static VALUE deq_options_get_condition(VALUE self)
{
    deq_options_t *options = rbdpi_to_deq_options(self);
    const char *val;
    uint32_t len;

    CHK(dpiDeqOptions_getCondition(options->handle, &val, &len));
    return rb_external_str_new_with_enc(val, len, options->enc);
}

static VALUE deq_options_get_consumer_name(VALUE self)
{
    deq_options_t *options = rbdpi_to_deq_options(self);
    const char *val;
    uint32_t len;

    CHK(dpiDeqOptions_getConsumerName(options->handle, &val, &len));
    return rb_external_str_new_with_enc(val, len, options->enc);
}

static VALUE deq_options_get_correlation(VALUE self)
{
    deq_options_t *options = rbdpi_to_deq_options(self);
    const char *val;
    uint32_t len;

    CHK(dpiDeqOptions_getCorrelation(options->handle, &val, &len));
    return rb_external_str_new_with_enc(val, len, options->enc);
}

static VALUE deq_options_get_mode(VALUE self)
{
    deq_options_t *options = rbdpi_to_deq_options(self);
    dpiDeqMode val;

    CHK(dpiDeqOptions_getMode(options->handle, &val));
    return rbdpi_from_dpiDeqMode(val);
}

static VALUE deq_options_get_msg_id(VALUE self)
{
    deq_options_t *options = rbdpi_to_deq_options(self);
    const char *val;
    uint32_t len;

    CHK(dpiDeqOptions_getMsgId(options->handle, &val, &len));
    return rb_external_str_new_with_enc(val, len, options->enc);
}

static VALUE deq_options_get_navigation(VALUE self)
{
    deq_options_t *options = rbdpi_to_deq_options(self);
    dpiDeqNavigation val;

    CHK(dpiDeqOptions_getNavigation(options->handle, &val));
    return rbdpi_from_dpiDeqNavigation(val);
}

static VALUE deq_options_get_transformation(VALUE self)
{
    deq_options_t *options = rbdpi_to_deq_options(self);
    const char *val;
    uint32_t len;

    CHK(dpiDeqOptions_getTransformation(options->handle, &val, &len));
    return rb_external_str_new_with_enc(val, len, options->enc);
}

static VALUE deq_options_get_visibility(VALUE self)
{
    deq_options_t *options = rbdpi_to_deq_options(self);
    dpiVisibility val;

    CHK(dpiDeqOptions_getVisibility(options->handle, &val));
    return rbdpi_from_dpiVisibility(val);
}

static VALUE deq_options_get_wait(VALUE self)
{
    deq_options_t *options = rbdpi_to_deq_options(self);
    uint32_t val;

    CHK(dpiDeqOptions_getWait(options->handle, &val));
    return UINT2NUM(val);
}

static VALUE deq_options_set_condition(VALUE self, VALUE val)
{
    deq_options_t *options = rbdpi_to_deq_options(self);

    CHK_STR_ENC(val, options->enc);
    CHK(dpiDeqOptions_setCondition(options->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    return val;
}

static VALUE deq_options_set_consumer_name(VALUE self, VALUE val)
{
    deq_options_t *options = rbdpi_to_deq_options(self);

    CHK_STR_ENC(val, options->enc);
    CHK(dpiDeqOptions_setConsumerName(options->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    return val;
}
static VALUE deq_options_set_correlation(VALUE self, VALUE val)
{
    deq_options_t *options = rbdpi_to_deq_options(self);

    CHK_STR_ENC(val, options->enc);
    CHK(dpiDeqOptions_setCorrelation(options->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    return val;
}

static VALUE deq_options_set_delivery_mode(VALUE self, VALUE val)
{
    deq_options_t *options = rbdpi_to_deq_options(self);

    CHK(dpiDeqOptions_setDeliveryMode(options->handle, rbdpi_to_dpiMessageDeliveryMode(val)));
    return val;
}

static VALUE deq_options_set_mode(VALUE self, VALUE val)
{
    deq_options_t *options = rbdpi_to_deq_options(self);

    CHK(dpiDeqOptions_setMode(options->handle, rbdpi_to_dpiDeqMode(val)));
    return val;
}

static VALUE deq_options_set_msg_id(VALUE self, VALUE val)
{
    deq_options_t *options = rbdpi_to_deq_options(self);

    CHK_STR_ENC(val, options->enc);
    CHK(dpiDeqOptions_setMsgId(options->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    return val;
}

static VALUE deq_options_set_navigation(VALUE self, VALUE val)
{
    deq_options_t *options = rbdpi_to_deq_options(self);

    CHK(dpiDeqOptions_setNavigation(options->handle, rbdpi_to_dpiDeqNavigation(val)));
    return val;
}

static VALUE deq_options_set_transformation(VALUE self, VALUE val)
{
    deq_options_t *options = rbdpi_to_deq_options(self);

    CHK_STR_ENC(val, options->enc);
    CHK(dpiDeqOptions_setTransformation(options->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    return val;
}

static VALUE deq_options_set_visibility(VALUE self, VALUE val)
{
    deq_options_t *options = rbdpi_to_deq_options(self);

    CHK(dpiDeqOptions_setVisibility(options->handle, rbdpi_to_dpiVisibility(val)));
    return val;
}

static VALUE deq_options_set_wait(VALUE self, VALUE val)
{
    deq_options_t *options = rbdpi_to_deq_options(self);

    CHK(dpiDeqOptions_setWait(options->handle, NUM2UINT(val)));
    return val;
}

void Init_rbdpi_deq_options(VALUE mDpi)
{
    cDeqOptions = rb_define_class_under(mDpi, "DeqOptions", rb_cObject);
    rb_define_alloc_func(cDeqOptions, deq_options_alloc);
    rb_define_method(cDeqOptions, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cDeqOptions, "initialize_copy", deq_options_initialize_copy, 1);
    rb_define_method(cDeqOptions, "condition", deq_options_get_condition, 0);
    rb_define_method(cDeqOptions, "consumer_name", deq_options_get_consumer_name, 0);
    rb_define_method(cDeqOptions, "correlation", deq_options_get_correlation, 0);
    rb_define_method(cDeqOptions, "mode", deq_options_get_mode, 0);
    rb_define_method(cDeqOptions, "msg_id", deq_options_get_msg_id, 0);
    rb_define_method(cDeqOptions, "navigation", deq_options_get_navigation, 0);
    rb_define_method(cDeqOptions, "transformation", deq_options_get_transformation, 0);
    rb_define_method(cDeqOptions, "visibility", deq_options_get_visibility, 0);
    rb_define_method(cDeqOptions, "wait", deq_options_get_wait, 0);
    rb_define_method(cDeqOptions, "condition=", deq_options_set_condition, 1);
    rb_define_method(cDeqOptions, "consumer_name=", deq_options_set_consumer_name, 1);
    rb_define_method(cDeqOptions, "correlation=", deq_options_set_correlation, 1);
    rb_define_method(cDeqOptions, "delivery_mode=", deq_options_set_delivery_mode, 1);
    rb_define_method(cDeqOptions, "mode=", deq_options_set_mode, 1);
    rb_define_method(cDeqOptions, "msg_id=", deq_options_set_msg_id, 1);
    rb_define_method(cDeqOptions, "navigation=", deq_options_set_navigation, 1);
    rb_define_method(cDeqOptions, "transformation=", deq_options_set_transformation, 1);
    rb_define_method(cDeqOptions, "visibility=", deq_options_set_visibility, 1);
    rb_define_method(cDeqOptions, "wait=", deq_options_set_wait, 1);
}

VALUE rbdpi_from_deq_options(dpiDeqOptions *handle, rb_encoding *enc)
{
    deq_options_t *options;
    VALUE obj = TypedData_Make_Struct(cDeqOptions, deq_options_t, &deq_options_data_type, options);

    options->handle = handle;
    options->enc = enc;
    return obj;
}

deq_options_t *rbdpi_to_deq_options(VALUE obj)
{
    deq_options_t *deq_options = (deq_options_t *)rb_check_typeddata(obj, &deq_options_data_type);

    if (deq_options->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return deq_options;
}
