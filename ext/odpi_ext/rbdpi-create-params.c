/*
 * rbdpi-create-params.c -- part of ruby-odpi
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

#define HAS_VALUE(v) ((v) != Qundef && !NIL_P(v))

static VALUE default_encoding;
static VALUE default_nencoding;
static VALUE ruby_to_oracle_encoding_map;

static VALUE get_default_encoding(VALUE module)
{
    return default_encoding;
}

static VALUE set_default_encoding(VALUE module, VALUE enc)
{
    rb_encoding *rbenc = rb_find_encoding(enc);
    default_encoding = rb_usascii_str_new_cstr(rb_enc_name(rbenc));
    RB_OBJ_FREEZE(default_encoding);
    return Qnil;
}

static VALUE get_default_nencoding(VALUE module)
{
    return default_nencoding;
}

static VALUE set_default_nencoding(VALUE module, VALUE enc)
{
    rb_encoding *rbenc = rb_find_encoding(enc);
    default_nencoding = rb_usascii_str_new_cstr(rb_enc_name(rbenc));
    RB_OBJ_FREEZE(default_nencoding);
    return Qnil;
}

void Init_rbdpi_create_params(VALUE mODPI)
{
    default_encoding = rb_usascii_str_new_cstr("UTF-8");
    RB_OBJ_FREEZE(default_encoding);
    default_nencoding = rb_usascii_str_new_cstr("UTF-8");
    RB_OBJ_FREEZE(default_nencoding);
    ruby_to_oracle_encoding_map = rb_hash_new();

    rb_global_variable(&default_encoding);
    rb_global_variable(&default_nencoding);

    rb_define_const(mODPI, "RUBY_TO_ORACLE_ENCODING", ruby_to_oracle_encoding_map);

    rb_define_module_function(mODPI, "default_encoding", get_default_encoding, 0);
    rb_define_module_function(mODPI, "default_encoding=", set_default_encoding, 1);
    rb_define_module_function(mODPI, "default_nencoding", get_default_nencoding, 0);
    rb_define_module_function(mODPI, "default_nencoding=", set_default_nencoding, 1);
}

rbdpi_enc_t rbdpi_get_encodings(VALUE params)
{
    VALUE enc = default_encoding;
    VALUE nenc = default_nencoding;
    rbdpi_enc_t rbenc;

    if (!NIL_P(params)) {
        VALUE val;

        val = rb_hash_aref(params, rbdpi_sym_encoding);
        if (!NIL_P(val)) {
            enc = val;
        }
        val = rb_hash_aref(params, rbdpi_sym_nencoding);
        if (!NIL_P(val)) {
            nenc = val;
        }
    }
    rbenc.enc = rb_find_encoding(enc);
    rbenc.nenc = rb_find_encoding(nenc);
    return rbenc;
}

static VALUE to_oracle_enc(const char **out, rb_encoding *rbenc)
{
    VALUE name = rb_usascii_str_new_cstr(rb_enc_name(rbenc));
    VALUE val = rb_hash_aref(ruby_to_oracle_encoding_map, name);
    if (NIL_P(val)) {
        *out = rb_enc_name(rbenc);
        return Qnil;
    } else {
        *out = StringValueCStr(val);
        return val;
    }
}

VALUE rbdpi_fill_dpiCommonCreateParams(dpiCommonCreateParams *dpi_params, VALUE params, rb_encoding *enc)
{
    static ID keyword_ids[4];
    VALUE kwargs[4];
    VALUE val;
    VALUE ary;

    CHK(dpiContext_initCommonCreateParams(rbdpi_g_context, dpi_params));
    dpi_params->createMode = DPI_MODE_CREATE_THREADED;

    if (NIL_P(params)) {
        return Qnil;
    }
    ary = rb_ary_new_capa(4);

    if (keyword_ids[0] == 0) {
        keyword_ids[0] = rb_intern("event");
        keyword_ids[1] = rb_intern("nencoding");
        keyword_ids[2] = rb_intern("edition");
        keyword_ids[3] = rb_intern("driver_name");
    }
    rb_get_kwargs(params, keyword_ids, 0, -4-1, kwargs);
    /* event */
    if (kwargs[0] != Qundef) {
        if (RTEST(kwargs[0])) {
            dpi_params->createMode |= DPI_MODE_CREATE_EVENTS;
        }
    }
    /* encoding */
    rb_ary_push(ary, to_oracle_enc(&dpi_params->encoding, enc));
    /* nencoding */
    val = HAS_VALUE(kwargs[1]) ? kwargs[1] : default_nencoding;
    rb_ary_push(ary, to_oracle_enc(&dpi_params->nencoding, rb_find_encoding(val)));
    /* edition */
    if (HAS_VALUE(kwargs[2])) {
        VALUE val = kwargs[2];
        CHK_STR_ENC(val, enc);
        dpi_params->edition = RSTRING_PTR(val);
        dpi_params->editionLength = RSTRING_LEN(val);
        rb_ary_push(ary, val);
    }
    /* driver_name */
    if (HAS_VALUE(kwargs[3])) {
        VALUE val = kwargs[3];
        CHK_STR_ENC(val, enc);
        dpi_params->driverName = RSTRING_PTR(val);
        dpi_params->driverNameLength = RSTRING_LEN(val);
        rb_ary_push(ary, val);
    } else {
        dpi_params->driverName = DEFAULT_DRIVER_NAME;
        dpi_params->driverNameLength = sizeof(DEFAULT_DRIVER_NAME) - 1;
    }
    return ary;
}

VALUE rbdpi_fill_dpiConnCreateParams(dpiConnCreateParams *dpi_params, VALUE params, VALUE auth_mode, rb_encoding *enc)
{
    static ID keyword_ids[6];
    VALUE kwargs[6];
    VALUE ary;

    CHK(dpiContext_initConnCreateParams(rbdpi_g_context, dpi_params));
    dpi_params->authMode = rbdpi_to_dpiAuthMode(auth_mode);
    if (NIL_P(params)) {
        return Qnil;
    }
    if (keyword_ids[0] == 0) {
        keyword_ids[0] = rb_intern("connection_class");
        keyword_ids[1] = rb_intern("purity");
        keyword_ids[2] = rb_intern("new_password");
        keyword_ids[3] = rb_intern("app_context");
        keyword_ids[4] = rb_intern("tag");
        keyword_ids[5] = rb_intern("match_any_tag");
    }
    rb_get_kwargs(params, keyword_ids, 0, -6-1, kwargs);
    if (rb_type_p(kwargs[3], T_ARRAY)) {
        ary = rb_ary_new_capa(4 + RARRAY_LEN(kwargs[3]) * 3);
    } else {
        ary = rb_ary_new_capa(3);
    }
    /* connection_class */
    if (HAS_VALUE(kwargs[0])) {
        VALUE val = kwargs[0];
        CHK_STR_ENC(val, enc);
        dpi_params->connectionClass = RSTRING_PTR(val);
        dpi_params->connectionClassLength = RSTRING_LEN(val);
        rb_ary_push(ary, val);
    }
    /* purity */
    if (HAS_VALUE(kwargs[1])) {
        dpi_params->purity = rbdpi_to_dpiPurity(kwargs[1]);
    }
    /* new_password */
    if (HAS_VALUE(kwargs[2])) {
        VALUE val = kwargs[2];
        CHK_STR_ENC(val, enc);
        dpi_params->newPassword = RSTRING_PTR(val);
        dpi_params->newPasswordLength = RSTRING_LEN(val);
        rb_ary_push(ary, val);
    }
    /* app_context */
    if (HAS_VALUE(kwargs[3])) {
        VALUE val = kwargs[3];
        long len, idx;
        VALUE tmp;

        if (!rb_type_p(val, T_ARRAY)) {
            rb_raise(rb_eArgError, "Invalid app_context format %s (expect Array)",
                     rb_obj_classname(val));
        }
        len = RARRAY_LEN(val);
        tmp = rb_str_tmp_new(len * sizeof(dpiAppContext));
        dpi_params->appContext = (dpiAppContext *)RSTRING_PTR(tmp);
        dpi_params->numAppContext = len;
        rb_ary_push(ary, tmp);
        for (idx = 0; idx < len; idx++) {
            VALUE elem = RARRAY_AREF(val, idx);

            if (!rb_type_p(elem, T_ARRAY)) {
                rb_raise(rb_eArgError, "Invalid app_context element %s (expect Array)",
                         rb_obj_classname(elem));
            }
            if (RARRAY_LEN(elem) != 3) {
                rb_raise(rb_eArgError, "invalid app_context element format");
            }
            tmp = RARRAY_AREF(elem, 0);
            CHK_STR_ENC(tmp, enc);
            dpi_params->appContext[idx].namespaceName = RSTRING_PTR(tmp);
            dpi_params->appContext[idx].namespaceNameLength = RSTRING_LEN(tmp);
            rb_ary_push(ary, tmp);

            tmp = RARRAY_AREF(elem, 1);
            CHK_STR_ENC(tmp, enc);
            dpi_params->appContext[idx].name = RSTRING_PTR(tmp);
            dpi_params->appContext[idx].nameLength = RSTRING_LEN(tmp);
            rb_ary_push(ary, tmp);

            tmp = RARRAY_AREF(elem, 2);
            CHK_STR_ENC(tmp, enc);
            dpi_params->appContext[idx].value = RSTRING_PTR(tmp);
            dpi_params->appContext[idx].valueLength = RSTRING_LEN(tmp);
            rb_ary_push(ary, tmp);
        }
    }
    /* tag */
    if (HAS_VALUE(kwargs[4])) {
        VALUE val = kwargs[4];
        CHK_STR_ENC(val, enc);
        dpi_params->tag = RSTRING_PTR(val);
        dpi_params->tagLength = RSTRING_LEN(val);
        rb_ary_push(ary, val);
    }
    /* match_any_tag */
    if (HAS_VALUE(kwargs[5])) {
        dpi_params->matchAnyTag = RTEST(kwargs[4]) ? 1 : 0;
    }
    return ary;
}

VALUE rbdpi_fill_dpiPoolCreateParams(dpiPoolCreateParams *dpi_params, VALUE params, rb_encoding *enc)
{
    static ID keyword_ids[7];
    VALUE kwargs[7];

    CHK(dpiContext_initPoolCreateParams(rbdpi_g_context, dpi_params));
    if (NIL_P(params)) {
        return Qnil;
    }
    if (keyword_ids[0] == 0) {
        keyword_ids[0] = rb_intern("min_sessions");
        keyword_ids[1] = rb_intern("max_sessions");
        keyword_ids[2] = rb_intern("session_increment");
        keyword_ids[3] = rb_intern("ping_interval");
        keyword_ids[4] = rb_intern("ping_timeout");
        keyword_ids[5] = rb_intern("homogeneous");
        keyword_ids[6] = rb_intern("get_mode");
    }
    rb_get_kwargs(params, keyword_ids, 0, -7-1, kwargs);
    /* min_sessions */
    if (HAS_VALUE(kwargs[0])) {
        dpi_params->minSessions = NUM2UINT(kwargs[0]);
    }
    /* max_sessions */
    if (HAS_VALUE(kwargs[1])) {
        dpi_params->maxSessions = NUM2UINT(kwargs[1]);
    }
    /* session_increment */
    if (HAS_VALUE(kwargs[2])) {
        dpi_params->sessionIncrement = NUM2UINT(kwargs[2]);
    }
    /* ping_interval */
    if (HAS_VALUE(kwargs[3])) {
        dpi_params->pingInterval = NUM2INT(kwargs[3]);
    }
    /* ping_timeout */
    if (HAS_VALUE(kwargs[4])) {
        dpi_params->pingTimeout = NUM2INT(kwargs[4]);
    }
    /* homogeneous */
    if (HAS_VALUE(kwargs[5])) {
        dpi_params->homogeneous = RTEST(kwargs[5]) ? 1 : 0;
    }
    /* get_mode */
    if (HAS_VALUE(kwargs[6])) {
        dpi_params->getMode = rbdpi_to_dpiPoolGetMode(kwargs[6]);
    }
    return Qnil;
}

VALUE rbdpi_fill_dpiSubscrCreateParams(dpiSubscrCreateParams *dpi_params, VALUE params, rb_encoding *enc)
{
    static ID keyword_ids[9];
    VALUE kwargs[9];
    VALUE ary;

    CHK(dpiContext_initSubscrCreateParams(rbdpi_g_context, dpi_params));
    if (NIL_P(params)) {
        return Qnil;
    }
    if (keyword_ids[0] == 0) {
        keyword_ids[0] = rb_intern("namespace");
        keyword_ids[1] = rb_intern("protocol");
        keyword_ids[2] = rb_intern("qos");
        keyword_ids[3] = rb_intern("operations");
        keyword_ids[4] = rb_intern("port");
        keyword_ids[5] = rb_intern("timeout");
        keyword_ids[6] = rb_intern("name");
        keyword_ids[7] = rb_intern("callback");
        keyword_ids[8] = rb_intern("recipient");
    }
    rb_get_kwargs(params, keyword_ids, 0, -9-1, kwargs);
    ary = rb_ary_new_capa(2);

    /* namespace */
    if (HAS_VALUE(kwargs[0])) {
        dpi_params->subscrNamespace = rbdpi_to_dpiSubscrNamespace(kwargs[0]);
    }
    /* protocol */
    if (HAS_VALUE(kwargs[1])) {
        dpi_params->protocol = rbdpi_to_dpiSubscrProtocol(kwargs[1]);
    }
    /* qos */
    if (HAS_VALUE(kwargs[2])) {
        dpi_params->qos = rbdpi_to_dpiSubscrQOS(kwargs[2]);
    }
    /* operations */
    if (HAS_VALUE(kwargs[3])) {
        dpi_params->operations = rbdpi_to_dpiOpCode(kwargs[3]);
    }
    /* port */
    if (HAS_VALUE(kwargs[4])) {
        dpi_params->operations = NUM2UINT(kwargs[4]);
    }
    /* timeout */
    if (HAS_VALUE(kwargs[5])) {
        dpi_params->operations = NUM2UINT(kwargs[5]);
    }
    /* name */
    if (HAS_VALUE(kwargs[6])) {
        VALUE val = kwargs[6];
        CHK_STR_ENC(val, enc);
        dpi_params->name = RSTRING_PTR(val);
        dpi_params->nameLength = RSTRING_LEN(val);
        rb_ary_push(ary, val);
    }
    /* callback */
    if (HAS_VALUE(kwargs[7])) {
        VALUE val = kwargs[7];
        if (!rb_obj_is_proc(val)) {
            rb_raise(rb_eTypeError, "wrong callback type %s (expect Proc)",
                     rb_obj_classname(val));
        }
        dpi_params->callback = (void*)val;
    }
    /* recipient */
    if (HAS_VALUE(kwargs[8])) {
        VALUE val = kwargs[8];
        CHK_STR_ENC(val, enc);
        dpi_params->recipientName = RSTRING_PTR(val);
        dpi_params->recipientNameLength = RSTRING_LEN(val);
        rb_ary_push(ary, val);
    }
    return ary;
}

