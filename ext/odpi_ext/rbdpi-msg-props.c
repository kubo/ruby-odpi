/*
 * rbdpi-msg-props.c -- part of ruby-odpi
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

static VALUE cMsgProps;

static void msg_props_free(void *arg)
{
    dpiMsgProps_release(((msg_props_t*)arg)->handle);
    xfree(arg);
}

static const struct rb_data_type_struct msg_props_data_type = {
    "ODPI::Dpi::MsgProps",
    {NULL, msg_props_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE msg_props_alloc(VALUE klass)
{
    msg_props_t *msg_props;

    return TypedData_Make_Struct(klass, msg_props_t, &msg_props_data_type, msg_props);
}

static VALUE msg_props_initialize_copy(VALUE self, VALUE other)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);

    *msg_props = *rbdpi_to_msg_props(other);
    if (msg_props->handle != NULL) {
        CHK(dpiMsgProps_addRef(msg_props->handle));
    }
    return self;
}

static VALUE msg_props_get_num_attempts(VALUE self)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);
    int32_t val;

    CHK(dpiMsgProps_getNumAttempts(msg_props->handle, &val));
    return INT2NUM(val);
}

static VALUE msg_props_get_correlation(VALUE self)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);
    const char *val;
    uint32_t len;

    CHK(dpiMsgProps_getCorrelation(msg_props->handle, &val, &len));
    return rb_external_str_new_with_enc(val, len, msg_props->enc);
}

static VALUE msg_props_get_delay(VALUE self)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);
    int32_t val;

    CHK(dpiMsgProps_getDelay(msg_props->handle, &val));
    return INT2NUM(val);
}

static VALUE msg_props_get_delivery_mode(VALUE self)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);
    dpiMessageDeliveryMode val;

    CHK(dpiMsgProps_getDeliveryMode(msg_props->handle, &val));
    return rbdpi_from_dpiMessageDeliveryMode(val);
}

static VALUE msg_props_get_enq_time(VALUE self)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);
    dpiTimestamp val;

    CHK(dpiMsgProps_getEnqTime(msg_props->handle, &val));
    return rbdpi_from_dpiTimestamp(&val);
}

static VALUE msg_props_get_exception_q(VALUE self)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);
    const char *val;
    uint32_t len;

    CHK(dpiMsgProps_getExceptionQ(msg_props->handle, &val, &len));
    return rb_external_str_new_with_enc(val, len, msg_props->enc);
}

static VALUE msg_props_get_expiration(VALUE self)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);
    int32_t val;

    CHK(dpiMsgProps_getExpiration(msg_props->handle, &val));
    return INT2NUM(val);
}

static VALUE msg_props_get_original_msg_id(VALUE self)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);
    const char *val;
    uint32_t len;

    CHK(dpiMsgProps_getOriginalMsgId(msg_props->handle, &val, &len));
    return rb_external_str_new_with_enc(val, len, msg_props->enc);
}

static VALUE msg_props_get_priority(VALUE self)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);
    int32_t val;

    CHK(dpiMsgProps_getPriority(msg_props->handle, &val));
    return INT2NUM(val);
}

static VALUE msg_props_get_state(VALUE self)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);
    dpiMessageState val;

    CHK(dpiMsgProps_getState(msg_props->handle, &val));
    return rbdpi_from_dpiMessageState(val);
}

static VALUE msg_props_set_correlation(VALUE self, VALUE val)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);

    CHK_STR_ENC(val, msg_props->enc);
    CHK(dpiMsgProps_setCorrelation(msg_props->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    return val;
}

static VALUE msg_props_set_delay(VALUE self, VALUE val)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);

    CHK(dpiMsgProps_setDelay(msg_props->handle, NUM2INT(val)));
    return val;
}

static VALUE msg_props_set_exception_q(VALUE self, VALUE val)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);

    CHK_STR_ENC(val, msg_props->enc);
    CHK(dpiMsgProps_setExceptionQ(msg_props->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    return val;
}

static VALUE msg_props_set_expiration(VALUE self, VALUE val)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);

    CHK(dpiMsgProps_setExpiration(msg_props->handle, NUM2INT(val)));
    return val;
}

static VALUE msg_props_set_original_msg_id(VALUE self, VALUE val)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);

    CHK_STR_ENC(val, msg_props->enc);
    CHK(dpiMsgProps_setOriginalMsgId(msg_props->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    return val;
}

static VALUE msg_props_set_priority(VALUE self, VALUE val)
{
    msg_props_t *msg_props = rbdpi_to_msg_props(self);

    CHK(dpiMsgProps_setPriority(msg_props->handle, NUM2INT(val)));
    return val;
}

void Init_rbdpi_msg_props(VALUE mDpi)
{
    cMsgProps = rb_define_class_under(mDpi, "MsgProps", rb_cObject);
    rb_define_alloc_func(cMsgProps, msg_props_alloc);
    rb_define_method(cMsgProps, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cMsgProps, "initialize_copy", msg_props_initialize_copy, 1);
    rb_define_method(cMsgProps, "num_attempts", msg_props_get_num_attempts, 0);
    rb_define_method(cMsgProps, "correlation", msg_props_get_correlation, 0);
    rb_define_method(cMsgProps, "delay", msg_props_get_delay, 0);
    rb_define_method(cMsgProps, "delivery_mode", msg_props_get_delivery_mode, 0);
    rb_define_method(cMsgProps, "enq_time", msg_props_get_enq_time, 0);
    rb_define_method(cMsgProps, "exception_q", msg_props_get_exception_q, 0);
    rb_define_method(cMsgProps, "expiration", msg_props_get_expiration, 0);
    rb_define_method(cMsgProps, "original_msg_id", msg_props_get_original_msg_id, 0);
    rb_define_method(cMsgProps, "priority", msg_props_get_priority, 0);
    rb_define_method(cMsgProps, "state", msg_props_get_state, 0);
    rb_define_method(cMsgProps, "correlation=", msg_props_set_correlation, 1);
    rb_define_method(cMsgProps, "delay=", msg_props_set_delay, 1);
    rb_define_method(cMsgProps, "exception_q=", msg_props_set_exception_q, 1);
    rb_define_method(cMsgProps, "expiration=", msg_props_set_expiration, 1);
    rb_define_method(cMsgProps, "original_msg_id=", msg_props_set_original_msg_id, 1);
    rb_define_method(cMsgProps, "priority=", msg_props_set_priority, 1);
}

VALUE rbdpi_from_msg_props(dpiMsgProps *handle, rb_encoding *enc)
{
    msg_props_t *msg_props;
    VALUE obj = TypedData_Make_Struct(cMsgProps, msg_props_t, &msg_props_data_type, msg_props);

    msg_props->handle = handle;
    msg_props->enc = enc;
    return obj;
}

msg_props_t *rbdpi_to_msg_props(VALUE obj)
{
    msg_props_t *msg_props = (msg_props_t *)rb_check_typeddata(obj, &msg_props_data_type);

    if (msg_props->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return msg_props;
}
