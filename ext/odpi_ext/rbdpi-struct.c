/*
 * rbdpi-struct.c -- part of ruby-odpi
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
 * ------------------------------------------------------
 * The license of documentation comments in this file is same with
 * ODPI-C because most of them were copied from ODPI-C documents.
 * The ODPI-C license is:
 *
 * Copyright (c) 2016, 2017 Oracle and/or its affiliates.  All rights reserved.
 * This program is free software: you can modify it and/or redistribute it
 * under the terms of:
 *
 * (i)  the Universal Permissive License v 1.0 or at your option, any
 *      later version (http://oss.oracle.com/licenses/upl); and/or
 *
 * (ii) the Apache License v 2.0. (http://www.apache.org/licenses/LICENSE-2.0)
 *
 * ------------------------------------------------------
 */
#include "rbdpi.h"

static VALUE default_driver_name;
static VALUE cCommonCreateParams;
static VALUE cConnCreateParams;
static VALUE cEncodingInfo;
static VALUE cError;
static VALUE cIntervalDS;
static VALUE cIntervalYM;
static VALUE cPoolCreateParams;
static VALUE cQueryInfo;
static VALUE cSubscrCreateParams;
static VALUE cTimestamp;

static ID id_at_name;
static ID id_at_type_info;

static inline VALUE check_safe_cstring_or_nil(VALUE val)
{
    if (NIL_P(val)) {
        return Qnil;
    }
    rb_check_safe_obj(val);
    StringValueCStr(val);
    return val;
}

static inline void check_and_create_str(VALUE *obj, const char *ptr, uint32_t len)
{
    if (NIL_P(*obj)) {
        if (ptr == 0) {
            return;
        }
    } else {
        Check_Type(obj, T_STRING);
        if (RSTRING_PTR(*obj) == ptr && RSTRING_LEN(*obj) == len) {
            return;
        }
    }
    *obj = rb_str_new(ptr, len);
}

/*
 * Document-class: ODPI::Dpi::CommonCreateParams
 *
 * This class is used for creating session pools and standalone connections to the database.
 *
 * @see ODPI::Dpi::Conn#initialize
 * @see ODPI::Dpi::Pool#initialize
 */
typedef struct {
    dpiCommonCreateParams params;
    VALUE encoding;
    VALUE nencoding;
    VALUE edition;
    VALUE driver_name;
} common_cp_t;

static void common_cp_mark(void *arg)
{
    common_cp_t *ccp = (common_cp_t*)arg;

    rb_gc_mark(ccp->encoding);
    rb_gc_mark(ccp->nencoding);
    rb_gc_mark(ccp->edition);
    rb_gc_mark(ccp->driver_name);
}

static size_t common_cp_memsize(const void *arg)
{
    return sizeof(common_cp_t);
}

static const struct rb_data_type_struct common_cp_data_type = {
    "ODPI::Dpi::CommonCreateParams",
    {common_cp_mark, RUBY_TYPED_DEFAULT_FREE, common_cp_memsize,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
#define TO_COMMON_CP(obj) ((common_cp_t *)rb_check_typeddata(obj, &common_cp_data_type))

static VALUE common_cp_alloc(VALUE klass)
{
    common_cp_t *ccp;
    VALUE obj = TypedData_Make_Struct(klass, common_cp_t, &common_cp_data_type, ccp);

    CHK(dpiContext_initCommonCreateParams(rbdpi_g_context, &ccp->params));
    ccp->params.createMode = DPI_MODE_CREATE_THREADED;
    ccp->encoding = Qnil;
    ccp->nencoding = Qnil;
    ccp->edition = Qnil;
    ccp->driver_name = default_driver_name;
    return obj;
}

/*
 * @private
 */
static VALUE common_cp_initialize_copy(VALUE self, VALUE other)
{
    *TO_COMMON_CP(self) = *TO_COMMON_CP(other);
    return self;
}

/*
 * @private
 */
static VALUE common_cp_inspect(VALUE self)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    return rb_sprintf("<%s: events=%s, encoding=%.*s, nencoding=%.*s, edition=%.*s, driver_name='%.*s'>",
                      rb_obj_classname(self),
                      (ccp->params.createMode & DPI_MODE_CREATE_EVENTS) ? "on" : "off",
                      NSTR_LEN_PTR_PAIR(ccp->encoding), NSTR_LEN_PTR_PAIR(ccp->nencoding),
                      NSTR_LEN_PTR_PAIR(ccp->edition), NSTR_LEN_PTR_PAIR(ccp->driver_name));
}

/*
 * Gets events mode which is required for the use of advanced queuing
 * (AQ) and continuous query notification (CQN).
 *
 * @return [Boolean]
 * @see #events=
 */
static VALUE common_cp_get_events(VALUE self)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    return (ccp->params.createMode & DPI_MODE_CREATE_EVENTS) ? Qtrue : Qfalse;
}

/*
 * Enables or disables events mode which is required for the use of
 * advanced queuing (AQ) and continuous query notification (CQN).
 *
 * @param mode [Boolean]
 */
static VALUE common_cp_set_events(VALUE self, VALUE mode)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    if (RTEST(mode)) {
        ccp->params.createMode |= DPI_MODE_CREATE_EVENTS;
    } else {
        ccp->params.createMode &= ~DPI_MODE_CREATE_EVENTS;
    }
    return mode;
}

/*
 * Gets the encoding to use for CHAR data.
 *
 * @return [String]
 * @see #encoding=
 */
static VALUE common_cp_get_encoding(VALUE self)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    return ccp->encoding;
}

/*
 * Specifies the encoding to use for CHAR data. Either an IANA or
 * Oracle specific character set name is expected. +nil+ is also
 * acceptable which implies the use of the NLS_LANG environment
 * variable (or ASCII, if the NLS_LANG environment variable is not
 * set). The default value is +nil+.
 *
 * @param name [String or nil] IANA or Oracle specific character set name
 */
static VALUE common_cp_set_encoding(VALUE self, VALUE name)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    ccp->encoding = check_safe_cstring_or_nil(name);
    return name;
}

/*
 * Gets the encoding to use for NCHAR data.
 *
 * @return [String or nil]
 * @see #nencoding=
 */
static VALUE common_cp_get_nencoding(VALUE self)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    return ccp->nencoding;
}

/*
 * Specifies the encoding to use for NCHAR data. Either an IANA or
 * Oracle specific character set name is expected. +nil+ is also
 * acceptable which implies the use of the NLS_NCHAR environment
 * variable (or the same value as the {#encoding} if the
 * NLS_NCHAR environment variable is not set). The default value is
 * +nil+.
 *
 * @param name [String or nil] IANA or Oracle specific character set name
 */
static VALUE common_cp_set_nencoding(VALUE self, VALUE name)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    ccp->nencoding = check_safe_cstring_or_nil(name);
    return name;
}

/*
 * Gets the edition to be used when creating a standalone
 * connection.
 *
 * @return [String or nil]
 */
static VALUE common_cp_get_edition(VALUE self)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    return ccp->edition;
}

/*
 * Specifies the edition to be used when creating a standalone
 * connection. It is expected to be +nil+ (meaning that no edition is
 * set) or a string in the encoding specified by the {#encoding}. The
 * default value is +nil+.
 *
 * @param edition [String or nil]
 */
static VALUE common_cp_set_edition(VALUE self, VALUE edition)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    CHK_NSTR(edition);
    ccp->edition = edition;
    return edition;
}

/*
 * Gets the name of the driver that is being used.
 *
 * @return [String or nil]
 */
static VALUE common_cp_get_driver_name(VALUE self)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    return ccp->driver_name;
}

/*
 * Specifies the name of the driver that is being used. It is expected
 * to be +nil+ or a string in the encoding specified by the
 * {#encoding}. The default value is "ruby-odpi_ext : version".
 *
 * @param name [String or nil]
 */
static VALUE common_cp_set_driver_name(VALUE self, VALUE name)
{
    common_cp_t *ccp = TO_COMMON_CP(self);

    CHK_NSTR(name);
    ccp->driver_name = name;
    return name;
}

/* ConnCreateParams */

typedef struct {
    dpiConnCreateParams params;
    VALUE connection_class;
    VALUE new_password;
    VALUE app_context;
    VALUE tag;
    VALUE out_tag;
    VALUE gc_guard;
} conn_cp_t;

static void conn_cp_mark(void *arg)
{
    conn_cp_t *ccp = (conn_cp_t*)arg;

    rb_gc_mark(ccp->connection_class);
    rb_gc_mark(ccp->new_password);
    rb_gc_mark(ccp->app_context);
    rb_gc_mark(ccp->tag);
    rb_gc_mark(ccp->out_tag);
    rb_gc_mark(ccp->gc_guard);
}

static size_t conn_cp_memsize(const void *arg)
{
    return sizeof(conn_cp_t);
}

static const struct rb_data_type_struct conn_cp_data_type = {
    "ODPI::Dpi::ConnCreateParams",
    {conn_cp_mark, RUBY_TYPED_DEFAULT_FREE, conn_cp_memsize,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
#define TO_CONN_CP(obj) ((conn_cp_t *)rb_check_typeddata(obj, &conn_cp_data_type))

static VALUE conn_cp_alloc(VALUE klass)
{
    conn_cp_t *ccp;
    VALUE obj = TypedData_Make_Struct(klass, conn_cp_t, &conn_cp_data_type, ccp);

    CHK(dpiContext_initConnCreateParams(rbdpi_g_context, &ccp->params));
    ccp->connection_class = Qnil;
    ccp->new_password = Qnil;
    ccp->app_context = Qnil;
    ccp->tag = Qnil;
    ccp->out_tag = Qnil;
    ccp->gc_guard = Qnil;
    return obj;
}

static VALUE conn_cp_initialize_copy(VALUE self, VALUE other)
{
    *TO_CONN_CP(self) = *TO_CONN_CP(other);
    return self;
}

static VALUE conn_cp_inspect(VALUE self)
{
    return rb_sprintf("<%s: ...>",
                      rb_obj_classname(self));
}

static VALUE conn_cp_get_auth_mode(VALUE self)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    return rbdpi_from_dpiAuthMode(ccp->params.authMode);
}

static VALUE conn_cp_set_auth_mode(VALUE self, VALUE val)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    ccp->params.authMode = rbdpi_to_dpiAuthMode(val);
    return val;
}

static VALUE conn_cp_get_connection_class(VALUE self)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    return ccp->connection_class;
}

static VALUE conn_cp_set_connection_class(VALUE self, VALUE val)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    CHK_NSTR(val);
    ccp->connection_class = val;
    return val;
}

static VALUE conn_cp_get_purity(VALUE self)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    return rbdpi_from_dpiPurity(ccp->params.purity);
}

static VALUE conn_cp_set_purity(VALUE self, VALUE val)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    ccp->params.purity = rbdpi_to_dpiPurity(val);
    return val;
}

static VALUE conn_cp_get_new_password(VALUE self)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    return ccp->new_password;
}

static VALUE conn_cp_set_new_password(VALUE self, VALUE val)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    CHK_NSTR(val);
    ccp->new_password = val;
    return val;
}

static VALUE conn_cp_get_app_context(VALUE self)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    return ccp->app_context;
}

static VALUE conn_cp_set_app_context(VALUE self, VALUE val)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    Check_Type(val, T_ARRAY);
    ccp->app_context = val;
    return val;
}

static VALUE conn_cp_get_external_auth(VALUE self)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    return ccp->params.externalAuth ? Qtrue : Qfalse;
}

static VALUE conn_cp_set_external_auth(VALUE self, VALUE val)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    ccp->params.externalAuth = RTEST(val) ? 1 : 0;
    return val;
}

static VALUE conn_cp_get_tag(VALUE self)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    return ccp->tag;
}

static VALUE conn_cp_set_tag(VALUE self, VALUE val)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    CHK_NSTR(val);
    ccp->tag = val;
    return val;
}

static VALUE conn_cp_get_match_any_tag(VALUE self)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    return ccp->params.matchAnyTag ? Qtrue : Qfalse;
}

static VALUE conn_cp_set_match_any_tag(VALUE self, VALUE val)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    ccp->params.matchAnyTag = RTEST(val) ? 1 : 0;
    return val;
}

static VALUE conn_cp_get_out_tag(VALUE self)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    return ccp->out_tag;
}

static VALUE conn_cp_get_tag_found_p(VALUE self)
{
    conn_cp_t *ccp = TO_CONN_CP(self);

    return ccp->params.outTagFound ? Qtrue : Qfalse;
}

/* EncodingInfo */
typedef struct {
    VALUE encoding;
    VALUE nencoding;
    int32_t maxBytesPerCharacter;
    int32_t nmaxBytesPerCharacter;
} enc_info_t;

static void enc_info_mark(void *arg)
{
    enc_info_t *ei = (enc_info_t *)arg;

    rb_gc_mark(ei->encoding);
    rb_gc_mark(ei->nencoding);
}

static size_t enc_info_memsize(const void *arg)
{
    return sizeof(enc_info_t);
}

static const struct rb_data_type_struct enc_info_data_type = {
    "ODPI::Dpi::EncodingInfo",
    {enc_info_mark, RUBY_TYPED_DEFAULT_FREE, enc_info_memsize,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
#define TO_ENC_INFO(obj) ((enc_info_t *)rb_check_typeddata(obj, &enc_info_data_type))

static VALUE enc_info_alloc(VALUE klass)
{
    enc_info_t *ei;
    VALUE obj = TypedData_Make_Struct(klass, enc_info_t, &enc_info_data_type, ei);

    ei->encoding = Qnil;
    ei->nencoding = Qnil;
    return obj;
}

static VALUE enc_info_initialize_copy(VALUE self, VALUE other)
{
    *TO_ENC_INFO(self) = *TO_ENC_INFO(other);
    return self;
}

static VALUE enc_info_inspect(VALUE self)
{
    enc_info_t *ei = TO_ENC_INFO(self);

    return rb_sprintf("<%s: enc=%.*s, max_byts_per_char=%d, nenc=%.*s, nmax_byts_per_char=%d>",
                      rb_obj_classname(self),
                      NSTR_LEN_PTR_PAIR(ei->encoding),
                      ei->maxBytesPerCharacter,
                      NSTR_LEN_PTR_PAIR(ei->nencoding),
                      ei->nmaxBytesPerCharacter);
}

static VALUE enc_info_get_encoding(VALUE self)
{
    enc_info_t *ei = TO_ENC_INFO(self);

    return ei->encoding;
}

static VALUE enc_info_get_max_bytes_per_char(VALUE self)
{
    enc_info_t *ei = TO_ENC_INFO(self);

    return INT2FIX(ei->maxBytesPerCharacter);
}

static VALUE enc_info_get_nencoding(VALUE self)
{
    enc_info_t *ei = TO_ENC_INFO(self);

    return ei->nencoding;
}

static VALUE enc_info_get_nmax_bytes_per_char(VALUE self)
{
    enc_info_t *ei = TO_ENC_INFO(self);

    return INT2FIX(ei->nmaxBytesPerCharacter);
}

/* Error */
typedef struct {
    int32_t code;
    uint16_t offset;
    VALUE fn_name;
    VALUE action;
    VALUE sql_state;
    int is_recoverable;
} error_t;

static void error_mark(void *arg)
{
    error_t *err = (error_t *)arg;

    rb_gc_mark(err->fn_name);
    rb_gc_mark(err->action);
    rb_gc_mark(err->sql_state);
}

static size_t error_memsize(const void *arg)
{
    return sizeof(error_t);
}

static const struct rb_data_type_struct error_data_type = {
    "ODPI::Dpi::Error",
    {error_mark, RUBY_TYPED_DEFAULT_FREE, error_memsize,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
#define TO_ERROR(obj) ((error_t *)rb_check_typeddata(obj, &error_data_type))

static VALUE error_alloc(VALUE klass)
{
    error_t *err;
    VALUE obj = TypedData_Make_Struct(klass, error_t, &error_data_type, err);

    err->fn_name = Qnil;
    err->action = Qnil;
    err->sql_state = Qnil;
    return obj;
}

static VALUE error_initialize(int argc, VALUE *argv, VALUE self)
{
    error_t *err = TO_ERROR(self);
    VALUE msg;
    VALUE code;
    VALUE offset;
    VALUE fn_name;
    VALUE action;
    VALUE sql_state;
    VALUE is_recoverable;

    rb_scan_args(argc, argv, "07", &msg, &code, &offset, &fn_name, &action, &sql_state, &is_recoverable);

    rb_call_super(argc ? 1 : 0, argv);
    CHK_NSTR(fn_name);
    CHK_NSTR(action);
    CHK_NSTR(sql_state);
    err->code = NIL_P(code) ? 0 : NUM2INT(code);
    err->offset = NIL_P(offset) ? 0 : NUM2UINT(offset);
    err->fn_name = fn_name;
    err->action = action;
    err->sql_state = sql_state;
    err->is_recoverable = RTEST(is_recoverable) ? 1 : 0;
    return self;
}

static VALUE error_initialize_copy(VALUE self, VALUE other)
{
    *TO_ERROR(self) = *TO_ERROR(other);
    return self;
}

static VALUE error_inspect(VALUE self)
{
    error_t *err = TO_ERROR(self);
    VALUE msg = rb_obj_as_string(self);

    return rb_sprintf("<%s: msg='%.*s', code=%u, offset=%d, fn_name=%.*s, action=%.*s, sql_state=%.*s, is_recoverable=%s>",
                      rb_obj_classname(self),
                      NSTR_LEN_PTR_PAIR(msg), err->code, err->offset,
                      NSTR_LEN_PTR_PAIR(err->fn_name), NSTR_LEN_PTR_PAIR(err->action),
                      NSTR_LEN_PTR_PAIR(err->sql_state), err->is_recoverable ? "true" : "false");
}

static VALUE error_get_code(VALUE self)
{
    error_t *err = TO_ERROR(self);

    return INT2NUM(err->code);
}

static VALUE error_get_offset(VALUE self)
{
    error_t *err = TO_ERROR(self);

    return INT2FIX(err->offset);
}

static VALUE error_get_fn_name(VALUE self)
{
    error_t *err = TO_ERROR(self);

    return err->fn_name;
}

static VALUE error_get_action(VALUE self)
{
    error_t *err = TO_ERROR(self);

    return err->action;
}

static VALUE error_get_sql_state(VALUE self)
{
    error_t *err = TO_ERROR(self);

    return err->sql_state;
}

static VALUE error_is_recoverable_p(VALUE self)
{
    error_t *err = TO_ERROR(self);

    return err->is_recoverable ? Qtrue : Qfalse;
}

/* IntervalDS */

static size_t intvl_ds_memsize(const void *arg)
{
    return sizeof(dpiIntervalDS);
}

static const struct rb_data_type_struct intvl_ds_data_type = {
    "ODPI::Dpi::IntervalDS",
    {NULL, RUBY_TYPED_DEFAULT_FREE, intvl_ds_memsize,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
#define TO_INTVL_DS(obj) ((dpiIntervalDS *)rb_check_typeddata(obj, &intvl_ds_data_type))

static VALUE intvl_ds_alloc(VALUE klass)
{
    dpiIntervalDS *intvl;

    return TypedData_Make_Struct(klass, dpiIntervalDS, &intvl_ds_data_type, intvl);
}

static VALUE intvl_ds_initialize(VALUE self, VALUE days, VALUE hours, VALUE minutes, VALUE seconds, VALUE fseconds)
{
    dpiIntervalDS intvl;

    intvl.days = NUM2INT(days);
    intvl.hours = NUM2INT(hours);
    intvl.minutes = NUM2INT(minutes);
    intvl.seconds = NUM2INT(seconds);
    intvl.fseconds = NUM2INT(fseconds);
    *TO_INTVL_DS(self) = intvl;
    return self;
}

static VALUE intvl_ds_initialize_copy(VALUE self, VALUE other)
{
    *TO_INTVL_DS(self) = *TO_INTVL_DS(other);
    return self;
}

static VALUE intvl_ds_inspect(VALUE self)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    return rb_sprintf("<%s: '%d %02d:%02d:%02d.%09d' day to second>",
                      rb_obj_classname(self),
                      intvl->days, intvl->hours, intvl->minutes,
                      intvl->seconds, intvl->fseconds);
}

static VALUE intvl_ds_get_days(VALUE self)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    return INT2NUM(intvl->days);
}

static VALUE intvl_ds_set_days(VALUE self, VALUE val)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    intvl->days = NUM2INT(val);
    return val;
}

static VALUE intvl_ds_get_hours(VALUE self)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    return INT2NUM(intvl->hours);
}

static VALUE intvl_ds_set_hours(VALUE self, VALUE val)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    intvl->hours = NUM2INT(val);
    return val;
}

static VALUE intvl_ds_get_minutes(VALUE self)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    return INT2NUM(intvl->minutes);
}

static VALUE intvl_ds_set_minutes(VALUE self, VALUE val)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    intvl->minutes = NUM2INT(val);
    return val;
}

static VALUE intvl_ds_get_seconds(VALUE self)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    return INT2NUM(intvl->seconds);
}

static VALUE intvl_ds_set_seconds(VALUE self, VALUE val)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    intvl->seconds = NUM2INT(val);
    return val;
}

static VALUE intvl_ds_get_fseconds(VALUE self)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    return INT2NUM(intvl->fseconds);
}

static VALUE intvl_ds_set_fseconds(VALUE self, VALUE val)
{
    dpiIntervalDS *intvl = TO_INTVL_DS(self);

    intvl->fseconds = NUM2INT(val);
    return val;
}

/* IntervalYM */

static size_t intvl_ym_memsize(const void *arg)
{
    return sizeof(dpiIntervalYM);
}

static const struct rb_data_type_struct intvl_ym_data_type = {
    "ODPI::Dpi::IntervalYM",
    {NULL, RUBY_TYPED_DEFAULT_FREE, intvl_ym_memsize,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
#define TO_INTVL_YM(obj) ((dpiIntervalYM *)rb_check_typeddata(obj, &intvl_ym_data_type))

static VALUE intvl_ym_alloc(VALUE klass)
{
    dpiIntervalYM *intvl;

    return TypedData_Make_Struct(klass, dpiIntervalYM, &intvl_ym_data_type, intvl);
}

static VALUE intvl_ym_initialize(VALUE self, VALUE years, VALUE months)
{
    dpiIntervalYM intvl;

    intvl.years = NUM2INT(years);
    intvl.months = NUM2INT(months);
    *TO_INTVL_YM(self) = intvl;
    return self;
}

static VALUE intvl_ym_initialize_copy(VALUE self, VALUE other)
{
    *TO_INTVL_YM(self) = *TO_INTVL_YM(other);
    return self;
}

static VALUE intvl_ym_inspect(VALUE self)
{
    dpiIntervalYM *intvl = TO_INTVL_YM(self);

    return rb_sprintf("<%s: '%d-%d' year to month>",
                      rb_obj_classname(self),
                      intvl->years, intvl->months);
}

static VALUE intvl_ym_get_years(VALUE self)
{
    dpiIntervalYM *intvl = TO_INTVL_YM(self);

    return INT2NUM(intvl->years);
}

static VALUE intvl_ym_set_years(VALUE self, VALUE val)
{
    dpiIntervalYM *intvl = TO_INTVL_YM(self);

    intvl->years = NUM2INT(val);
    return val;
}

static VALUE intvl_ym_get_months(VALUE self)
{
    dpiIntervalYM *intvl = TO_INTVL_YM(self);

    return INT2NUM(intvl->months);
}

static VALUE intvl_ym_set_months(VALUE self, VALUE val)
{
    dpiIntervalYM *intvl = TO_INTVL_YM(self);

    intvl->months = NUM2INT(val);
    return val;
}

/* PoolCreateParams */

typedef struct {
    dpiPoolCreateParams params;
    VALUE pool_name;
} pool_cp_t;

static void pool_cp_mark(void *arg)
{
    pool_cp_t *pcp = (pool_cp_t *)arg;

    rb_gc_mark(pcp->pool_name);
}

static size_t pool_cp_memsize(const void *arg)
{
    return sizeof(pool_cp_t);
}

static const struct rb_data_type_struct pool_cp_data_type = {
    "ODPI::Dpi::PoolCreateParams",
    {pool_cp_mark, RUBY_TYPED_DEFAULT_FREE, pool_cp_memsize,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
#define TO_POOL_CP(obj) ((pool_cp_t *)rb_check_typeddata(obj, &pool_cp_data_type))

static VALUE pool_cp_alloc(VALUE klass)
{
    pool_cp_t *pcp;
    VALUE obj = TypedData_Make_Struct(klass, pool_cp_t, &pool_cp_data_type, pcp);

    CHK(dpiContext_initPoolCreateParams(rbdpi_g_context, &pcp->params));
    pcp->pool_name = Qnil;
    return obj;
}

static VALUE pool_cp_initialize_copy(VALUE self, VALUE other)
{
    *TO_POOL_CP(self) = *TO_POOL_CP(other);
    return self;
}

static VALUE pool_cp_inspect(VALUE self)
{
    pool_cp_t *pcp = TO_POOL_CP(self);
    VALUE get_mode = rbdpi_from_dpiPoolGetMode(pcp->params.getMode);

    return rb_sprintf("<%s: sessions(max,min,incr)=%u,%u,%u, ping(interval,timeout)=%d,%d, homogeneous=%s, external_auth=%s, get_mode=%s, pool_name=%.*s>",
                      rb_obj_classname(self),
                      pcp->params.maxSessions, pcp->params.minSessions, pcp->params.sessionIncrement,
                      pcp->params.pingInterval, pcp->params.pingTimeout,
                      pcp->params.homogeneous ? "true" : "false",
                      pcp->params.externalAuth ? "true" : "false",
                      rb_id2name(SYM2ID(get_mode)),
                      NSTR_LEN_PTR_PAIR(pcp->pool_name));
}

static VALUE pool_cp_get_min_sessions(VALUE self)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    return UINT2NUM(pcp->params.minSessions);
}

static VALUE pool_cp_set_min_sessions(VALUE self, VALUE val)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    pcp->params.minSessions = NUM2UINT(val);
    return val;
}

static VALUE pool_cp_get_max_sessions(VALUE self)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    return UINT2NUM(pcp->params.maxSessions);
}

static VALUE pool_cp_set_max_sessions(VALUE self, VALUE val)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    pcp->params.maxSessions = NUM2UINT(val);
    return val;
}

static VALUE pool_cp_get_session_increment(VALUE self)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    return UINT2NUM(pcp->params.sessionIncrement);
}

static VALUE pool_cp_set_session_increment(VALUE self, VALUE val)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    pcp->params.sessionIncrement = NUM2UINT(val);
    return val;
}

static VALUE pool_cp_get_ping_interval(VALUE self)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    return INT2NUM(pcp->params.pingInterval);
}

static VALUE pool_cp_set_ping_interval(VALUE self, VALUE val)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    pcp->params.pingInterval = NUM2INT(val);
    return val;
}

static VALUE pool_cp_get_ping_timeout(VALUE self)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    return INT2NUM(pcp->params.pingTimeout);
}

static VALUE pool_cp_set_ping_timeout(VALUE self, VALUE val)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    pcp->params.pingTimeout = NUM2INT(val);
    return val;
}

static VALUE pool_cp_get_homogeneous(VALUE self)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    return pcp->params.homogeneous ? Qtrue : Qfalse;
}

static VALUE pool_cp_set_homogeneous(VALUE self, VALUE val)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    pcp->params.homogeneous = RTEST(val) ? 1 : 0;
    return val;
}

static VALUE pool_cp_get_external_auth(VALUE self)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    return pcp->params.externalAuth ? Qtrue : Qfalse;
}

static VALUE pool_cp_set_external_auth(VALUE self, VALUE val)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    pcp->params.externalAuth = RTEST(val) ? 1 : 0;
    return val;
}

static VALUE pool_cp_get_get_mode(VALUE self)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    return rbdpi_from_dpiPoolGetMode(pcp->params.getMode);
}

static VALUE pool_cp_set_get_mode(VALUE self, VALUE val)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    pcp->params.getMode = rbdpi_to_dpiPoolGetMode(val);
    return val;
}

static VALUE pool_cp_get_pool_name(VALUE self)
{
    pool_cp_t *pcp = TO_POOL_CP(self);

    return pcp->pool_name;
}

/*
 * Document-class: ODPI::Dpi::QueryInfo
 *
 * This class represents query metadata.
 *
 * @see ODPI::Dpi::Stmt#query_columns
 */

/*
 * @private
 */
static VALUE query_info_inspect(VALUE self)
{
    VALUE str = rb_str_buf_new2("<");

    rb_str_buf_append(str, rb_obj_class(self));
    rb_str_buf_cat_ascii(str, ": ");
    rb_str_buf_append(str, rb_ivar_get(self, id_at_name));
    rb_str_buf_cat_ascii(str, " ");
    rb_str_buf_append(str, rb_ivar_get(self, id_at_type_info));
    rb_str_buf_cat_ascii(str, ">");
    return str;
}

/*
 * Gets the name of the column which is being queried
 *
 * @return [String]
 */
static VALUE query_info_get_name(VALUE self)
{
    return rb_ivar_get(self, id_at_name);
}

/*
 * Gets the type of data of the column that is being queried.
 *
 * @return [ODPI::Dpi::DataType]
 */
static VALUE query_info_get_type_info(VALUE self)
{
    return rb_ivar_get(self, id_at_type_info);
}

/* SubscrCreateParams */

typedef struct {
    dpiSubscrCreateParams params;
    VALUE name;
    VALUE callback;
    VALUE recipient_name;
} subscr_cp_t;

static void subscr_cp_mark(void *arg)
{
    subscr_cp_t *scp = (subscr_cp_t*)arg;

    rb_gc_mark(scp->name);
    rb_gc_mark(scp->callback);
    rb_gc_mark(scp->recipient_name);
}

static size_t subscr_cp_memsize(const void *arg)
{
    return sizeof(subscr_cp_t);
}

static const struct rb_data_type_struct subscr_cp_data_type = {
    "ODPI::Dpi::SubscrCreateParams",
    {subscr_cp_mark, RUBY_TYPED_DEFAULT_FREE, subscr_cp_memsize,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
#define TO_SUBSCR_CP(obj) ((subscr_cp_t *)rb_check_typeddata(obj, &subscr_cp_data_type))

static VALUE subscr_cp_alloc(VALUE klass)
{
    subscr_cp_t *scp;
    VALUE obj = TypedData_Make_Struct(klass, subscr_cp_t, &subscr_cp_data_type, scp);

    CHK(dpiContext_initSubscrCreateParams(rbdpi_g_context, &scp->params));
    scp->name = Qnil;
    scp->callback = Qnil;
    scp->recipient_name = Qnil;
    return obj;
}

static VALUE subscr_cp_initialize_copy(VALUE self, VALUE other)
{
    *TO_SUBSCR_CP(self) = *TO_SUBSCR_CP(other);
    return self;
}

static VALUE subscr_cp_inspect(VALUE self)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);
    VALUE namespace = rbdpi_from_dpiSubscrNamespace(scp->params.subscrNamespace);
    VALUE protocol = rbdpi_from_dpiSubscrNamespace(scp->params.protocol);

    return rb_sprintf("<%s: subscr_namespace=%s, protocol=%s, ...>",
                      rb_obj_classname(self),
                      rb_id2name(ID2SYM(namespace)), rb_id2name(ID2SYM(protocol)));
}

static VALUE subscr_cp_get_subscr_namespace(VALUE self)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    return rbdpi_from_dpiSubscrNamespace(scp->params.subscrNamespace);
}

static VALUE subscr_cp_set_subscr_namespace(VALUE self, VALUE val)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    scp->params.subscrNamespace = rbdpi_to_dpiSubscrNamespace(val);
    return val;
}

static VALUE subscr_cp_get_protocol(VALUE self)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    return rbdpi_from_dpiSubscrProtocol(scp->params.protocol);
}

static VALUE subscr_cp_set_protocol(VALUE self, VALUE val)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    scp->params.protocol = rbdpi_to_dpiSubscrProtocol(val);
    return val;
}

static VALUE subscr_cp_get_qos(VALUE self)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    return rbdpi_from_dpiSubscrQOS(scp->params.qos);
}

static VALUE subscr_cp_set_qos(VALUE self, VALUE val)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    scp->params.qos = rbdpi_to_dpiSubscrQOS(val);
    return val;
}

static VALUE subscr_cp_get_operations(VALUE self)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    return rbdpi_from_dpiOpCode(scp->params.operations);
}

static VALUE subscr_cp_set_operations(VALUE self, VALUE val)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    scp->params.operations = rbdpi_to_dpiOpCode(val);
    return val;
}

static VALUE subscr_cp_get_port_number(VALUE self)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    return UINT2NUM(scp->params.portNumber);
}

static VALUE subscr_cp_set_port_number(VALUE self, VALUE val)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    scp->params.portNumber = NUM2UINT(val);
    return val;
}

static VALUE subscr_cp_get_timeout(VALUE self)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    return UINT2NUM(scp->params.timeout);
}

static VALUE subscr_cp_set_timeout(VALUE self, VALUE val)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    scp->params.timeout = NUM2UINT(val);
    return val;
}

static VALUE subscr_cp_get_name(VALUE self)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    return scp->name;
}

static VALUE subscr_cp_set_name(VALUE self, VALUE val)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    CHK_NSTR(val);
    scp->name = val;
    return val;
}

static VALUE subscr_cp_get_callback(VALUE self)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    return scp->callback;
}

static VALUE subscr_cp_set_callback(VALUE self, VALUE val)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    if (!rb_obj_is_proc(val)) {
        rb_raise(rb_eTypeError, "wrong type %s (expect Proc)",
                 rb_obj_classname(val));
    }
    scp->callback = val;
    return val;
}

static VALUE subscr_cp_get_recipient_name(VALUE self)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    return scp->recipient_name;
}

static VALUE subscr_cp_set_recipient_name(VALUE self, VALUE val)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(self);

    CHK_NSTR(val);
    scp->recipient_name = val;
    return val;
}

/* Timestamp */

static size_t timestamp_memsize(const void *arg)
{
    return sizeof(dpiTimestamp);
}

static const struct rb_data_type_struct timestamp_data_type = {
    "ODPI::Dpi::Timestamp",
    {NULL, RUBY_TYPED_DEFAULT_FREE, timestamp_memsize,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
#define TO_TIMESTAMP(obj) ((dpiTimestamp *)rb_check_typeddata(obj, &timestamp_data_type))

static VALUE timestamp_alloc(VALUE klass)
{
    dpiTimestamp *ts;
    VALUE obj = TypedData_Make_Struct(klass, dpiTimestamp, &timestamp_data_type, ts);
    ts->year = 1;
    ts->month = 1;
    return obj;
}

static VALUE timestamp_initialize(VALUE self, VALUE year, VALUE month, VALUE day, VALUE hour, VALUE minute, VALUE second, VALUE fsecond, VALUE tz_hour_offset, VALUE tz_minute_offset)
{
    dpiTimestamp ts;
    ts.year = NUM2INT(year);
    ts.month = NUM2INT(month);
    ts.day = NUM2INT(day);
    ts.hour = NUM2INT(hour);
    ts.minute = NUM2INT(minute);
    ts.second = NUM2INT(second);
    ts.fsecond = NUM2INT(fsecond);
    ts.tzHourOffset = NUM2INT(tz_hour_offset);
    ts.tzMinuteOffset = NUM2INT(tz_minute_offset);
    *TO_TIMESTAMP(self) = ts;
    return self;
}

static VALUE timestamp_initialize_copy(VALUE self, VALUE other)
{
    *TO_TIMESTAMP(self) = *TO_TIMESTAMP(other);
    return self;
}

static VALUE timestamp_inspect(VALUE self)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    return rb_sprintf("<%s: %d-%02d-%02d %02d:%02d:%02d.%09d %02d:%02d>",
                      rb_obj_classname(self),
                      ts->year, ts->month, ts->day,
                      ts->hour, ts->minute, ts->second, ts->fsecond,
                      ts->tzHourOffset, ts->tzMinuteOffset);
}

static VALUE timestamp_get_year(VALUE self)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    return INT2NUM(ts->year);
}

static VALUE timestamp_set_year(VALUE self, VALUE val)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    ts->year = NUM2INT(val);
    return val;
}

static VALUE timestamp_get_month(VALUE self)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    return INT2NUM(ts->month);
}

static VALUE timestamp_set_month(VALUE self, VALUE val)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    ts->month = NUM2INT(val);
    return val;
}

static VALUE timestamp_get_day(VALUE self)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    return INT2NUM(ts->day);
}

static VALUE timestamp_set_day(VALUE self, VALUE val)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    ts->day = NUM2INT(val);
    return val;
}

static VALUE timestamp_get_hour(VALUE self)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    return INT2NUM(ts->hour);
}

static VALUE timestamp_set_hour(VALUE self, VALUE val)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    ts->hour = NUM2INT(val);
    return val;
}

static VALUE timestamp_get_minute(VALUE self)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    return INT2NUM(ts->minute);
}

static VALUE timestamp_set_minute(VALUE self, VALUE val)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    ts->minute = NUM2INT(val);
    return val;
}

static VALUE timestamp_get_second(VALUE self)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    return INT2NUM(ts->second);
}

static VALUE timestamp_set_second(VALUE self, VALUE val)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    ts->second = NUM2INT(val);
    return val;
}

static VALUE timestamp_get_fsecond(VALUE self)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    return INT2NUM(ts->fsecond);
}

static VALUE timestamp_set_fsecond(VALUE self, VALUE val)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    ts->fsecond = NUM2INT(val);
    return val;
}

static VALUE timestamp_get_tz_hour_offset(VALUE self)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    return INT2NUM(ts->tzHourOffset);
}

static VALUE timestamp_set_tz_hour_offset(VALUE self, VALUE val)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    ts->tzHourOffset = NUM2INT(val);
    return val;
}

static VALUE timestamp_get_tz_minute_offset(VALUE self)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    return INT2NUM(ts->tzMinuteOffset);
}

static VALUE timestamp_set_tz_minute_offset(VALUE self, VALUE val)
{
    dpiTimestamp *ts = TO_TIMESTAMP(self);

    ts->tzMinuteOffset = NUM2INT(val);
    return val;
}

void Init_rbdpi_struct(VALUE mDpi)
{
    default_driver_name = rb_usascii_str_new_cstr(DEFAULT_DRIVER_NAME);
    OBJ_FREEZE(default_driver_name);
    rb_global_variable(&default_driver_name);

    id_at_name = rb_intern("@name");
    id_at_type_info = rb_intern("@type_info");

    /* CommonCreateParams */
    cCommonCreateParams = rb_define_class_under(mDpi, "CommonCreateParams", rb_cObject);
    rb_define_alloc_func(cCommonCreateParams, common_cp_alloc);
    //rb_define_method(cCommonCreateParams, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cCommonCreateParams, "initialize_copy", common_cp_initialize_copy, 1);
    rb_define_method(cCommonCreateParams, "inspect", common_cp_inspect, 0);
    rb_define_method(cCommonCreateParams, "events?", common_cp_get_events, 0);
    rb_define_method(cCommonCreateParams, "events=", common_cp_set_events, 1);
    rb_define_method(cCommonCreateParams, "encoding", common_cp_get_encoding, 0);
    rb_define_method(cCommonCreateParams, "encoding=", common_cp_set_encoding, 1);
    rb_define_method(cCommonCreateParams, "nencoding", common_cp_get_nencoding, 0);
    rb_define_method(cCommonCreateParams, "nencoding=", common_cp_set_nencoding, 1);
    rb_define_method(cCommonCreateParams, "edition", common_cp_get_edition, 0);
    rb_define_method(cCommonCreateParams, "edition=", common_cp_set_edition, 1);
    rb_define_method(cCommonCreateParams, "driver_name", common_cp_get_driver_name, 0);
    rb_define_method(cCommonCreateParams, "driver_name=", common_cp_set_driver_name, 1);

    /* ConnCreateParams */
    cConnCreateParams = rb_define_class_under(mDpi, "ConnCreateParams", rb_cObject);
    rb_define_alloc_func(cConnCreateParams, conn_cp_alloc);
    rb_define_method(cConnCreateParams, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cConnCreateParams, "initialize_copy", conn_cp_initialize_copy, 1);
    rb_define_method(cConnCreateParams, "inspect", conn_cp_inspect, 0);
    rb_define_method(cConnCreateParams, "auth_mode", conn_cp_get_auth_mode, 0);
    rb_define_method(cConnCreateParams, "auth_mode=", conn_cp_set_auth_mode, 1);
    rb_define_method(cConnCreateParams, "connection_class", conn_cp_get_connection_class, 0);
    rb_define_method(cConnCreateParams, "connection_class=", conn_cp_set_connection_class, 1);
    rb_define_method(cConnCreateParams, "purity", conn_cp_get_purity, 0);
    rb_define_method(cConnCreateParams, "purity=", conn_cp_set_purity, 1);
    rb_define_method(cConnCreateParams, "new_password", conn_cp_get_new_password, 0);
    rb_define_method(cConnCreateParams, "new_password=", conn_cp_set_new_password, 1);
    rb_define_method(cConnCreateParams, "app_context", conn_cp_get_app_context, 0);
    rb_define_method(cConnCreateParams, "app_context=", conn_cp_set_app_context, 1);
    rb_define_method(cConnCreateParams, "external_auth", conn_cp_get_external_auth, 0);
    rb_define_method(cConnCreateParams, "external_auth=", conn_cp_set_external_auth, 1);
    rb_define_method(cConnCreateParams, "tag", conn_cp_get_tag, 0);
    rb_define_method(cConnCreateParams, "tag=", conn_cp_set_tag, 1);
    rb_define_method(cConnCreateParams, "match_any_tag", conn_cp_get_match_any_tag, 0);
    rb_define_method(cConnCreateParams, "match_any_tag=", conn_cp_set_match_any_tag, 1);
    rb_define_method(cConnCreateParams, "out_tag", conn_cp_get_out_tag, 0);
    rb_define_method(cConnCreateParams, "tag_found?", conn_cp_get_tag_found_p, 0);

    /* EncodingInfo */
    cEncodingInfo = rb_define_class_under(mDpi, "EncodingInfo", rb_cObject);
    rb_define_alloc_func(cEncodingInfo, enc_info_alloc);
    rb_define_method(cEncodingInfo, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cEncodingInfo, "initialize_copy", enc_info_initialize_copy, 1);
    rb_define_method(cEncodingInfo, "inspect", enc_info_inspect, 0);
    rb_define_method(cEncodingInfo, "encoding", enc_info_get_encoding, 0);
    rb_define_method(cEncodingInfo, "max_bytes_per_char", enc_info_get_max_bytes_per_char, 0);
    rb_define_method(cEncodingInfo, "nencoding", enc_info_get_nencoding, 0);
    rb_define_method(cEncodingInfo, "nmax_bytes_per_char", enc_info_get_nmax_bytes_per_char, 0);

    /* Error */
    cError = rb_define_class_under(mDpi, "Error", rb_eStandardError);
    rb_define_alloc_func(cError, error_alloc);
    rb_define_method(cError, "initialize", error_initialize, -1);
    rb_define_method(cError, "initialize_copy", error_initialize_copy, 1);
    rb_define_method(cError, "inspect", error_inspect, 0);
    rb_define_method(cError, "code", error_get_code, 0);
    rb_define_method(cError, "offset", error_get_offset, 0);
    rb_define_method(cError, "fn_name", error_get_fn_name, 0);
    rb_define_method(cError, "action", error_get_action, 0);
    rb_define_method(cError, "sql_state", error_get_sql_state, 0);
    rb_define_method(cError, "is_recoverable?", error_is_recoverable_p, 0);

    /* IntervalDS */
    cIntervalDS = rb_define_class_under(mDpi, "IntervalDS", rb_cObject);
    rb_define_alloc_func(cIntervalDS, intvl_ds_alloc);
    rb_define_method(cIntervalDS, "initialize", intvl_ds_initialize, 5);
    rb_define_method(cIntervalDS, "initialize_copy", intvl_ds_initialize_copy, 1);
    rb_define_method(cIntervalDS, "inspect", intvl_ds_inspect, 0);
    rb_define_method(cIntervalDS, "days", intvl_ds_get_days, 0);
    rb_define_method(cIntervalDS, "days=", intvl_ds_set_days, 1);
    rb_define_method(cIntervalDS, "hours", intvl_ds_get_hours, 0);
    rb_define_method(cIntervalDS, "hours=", intvl_ds_set_hours, 1);
    rb_define_method(cIntervalDS, "minutes", intvl_ds_get_minutes, 0);
    rb_define_method(cIntervalDS, "minutes=", intvl_ds_set_minutes, 1);
    rb_define_method(cIntervalDS, "seconds", intvl_ds_get_seconds, 0);
    rb_define_method(cIntervalDS, "seconds=", intvl_ds_set_seconds, 1);
    rb_define_method(cIntervalDS, "fseconds", intvl_ds_get_fseconds, 0);
    rb_define_method(cIntervalDS, "fseconds=", intvl_ds_set_fseconds, 1);

    /* IntervalYM */
    cIntervalYM = rb_define_class_under(mDpi, "IntervalYM", rb_cObject);
    rb_define_alloc_func(cIntervalYM, intvl_ym_alloc);
    rb_define_method(cIntervalYM, "initialize", intvl_ym_initialize, 2);
    rb_define_method(cIntervalYM, "initialize_copy", intvl_ym_initialize_copy, 1);
    rb_define_method(cIntervalYM, "inspect", intvl_ym_inspect, 0);
    rb_define_method(cIntervalYM, "years", intvl_ym_get_years, 0);
    rb_define_method(cIntervalYM, "years=", intvl_ym_set_years, 1);
    rb_define_method(cIntervalYM, "months", intvl_ym_get_months, 0);
    rb_define_method(cIntervalYM, "months=", intvl_ym_set_months, 1);

    /* PoolCreateParams */
    cPoolCreateParams = rb_define_class_under(mDpi, "PoolCreateParams", rb_cObject);
    rb_define_alloc_func(cPoolCreateParams, pool_cp_alloc);
    rb_define_method(cPoolCreateParams, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cPoolCreateParams, "initialize_copy", pool_cp_initialize_copy, 1);
    rb_define_method(cPoolCreateParams, "inspect", pool_cp_inspect, 0);
    rb_define_method(cPoolCreateParams, "min_sessions", pool_cp_get_min_sessions, 0);
    rb_define_method(cPoolCreateParams, "min_sessions=", pool_cp_set_min_sessions, 1);
    rb_define_method(cPoolCreateParams, "max_sessions", pool_cp_get_max_sessions, 0);
    rb_define_method(cPoolCreateParams, "max_sessions=", pool_cp_set_max_sessions, 1);
    rb_define_method(cPoolCreateParams, "session_increment", pool_cp_get_session_increment, 0);
    rb_define_method(cPoolCreateParams, "session_increment=", pool_cp_set_session_increment, 1);
    rb_define_method(cPoolCreateParams, "ping_interval", pool_cp_get_ping_interval, 0);
    rb_define_method(cPoolCreateParams, "ping_interval=", pool_cp_set_ping_interval, 1);
    rb_define_method(cPoolCreateParams, "ping_timeout", pool_cp_get_ping_timeout, 0);
    rb_define_method(cPoolCreateParams, "ping_timeout=", pool_cp_set_ping_timeout, 1);
    rb_define_method(cPoolCreateParams, "homogeneous?", pool_cp_get_homogeneous, 0);
    rb_define_method(cPoolCreateParams, "homogeneous=", pool_cp_set_homogeneous, 1);
    rb_define_method(cPoolCreateParams, "external_auth?", pool_cp_get_external_auth, 0);
    rb_define_method(cPoolCreateParams, "external_auth=", pool_cp_set_external_auth, 1);
    rb_define_method(cPoolCreateParams, "get_mode", pool_cp_get_get_mode, 0);
    rb_define_method(cPoolCreateParams, "get_mode=", pool_cp_set_get_mode, 1);
    rb_define_method(cPoolCreateParams, "pool_name", pool_cp_get_pool_name, 0);

    /* QueryInfo */
    cQueryInfo = rb_define_class_under(mDpi, "QueryInfo", rb_cObject);
    rb_define_method(cQueryInfo, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cQueryInfo, "inspect", query_info_inspect, 0);
    rb_define_method(cQueryInfo, "name", query_info_get_name, 0);
    rb_define_method(cQueryInfo, "type_info", query_info_get_type_info, 0);

    /* SubscrCreateParams */
    cSubscrCreateParams = rb_define_class_under(mDpi, "SubscrCreateParams", rb_cObject);
    rb_define_alloc_func(cSubscrCreateParams, subscr_cp_alloc);
    //rb_define_method(cSubscrCreateParams, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cSubscrCreateParams, "initialize_copy", subscr_cp_initialize_copy, 1);
    rb_define_method(cSubscrCreateParams, "inspect", subscr_cp_inspect, 0);
    rb_define_method(cSubscrCreateParams, "subscr_namespace", subscr_cp_get_subscr_namespace, 0);
    rb_define_method(cSubscrCreateParams, "subscr_namespace=", subscr_cp_set_subscr_namespace, 1);
    rb_define_method(cSubscrCreateParams, "protocol", subscr_cp_get_protocol, 0);
    rb_define_method(cSubscrCreateParams, "protocol=", subscr_cp_set_protocol, 1);
    rb_define_method(cSubscrCreateParams, "qos", subscr_cp_get_qos, 0);
    rb_define_method(cSubscrCreateParams, "qos=", subscr_cp_set_qos, 1);
    rb_define_method(cSubscrCreateParams, "operations", subscr_cp_get_operations, 0);
    rb_define_method(cSubscrCreateParams, "operations=", subscr_cp_set_operations, 1);
    rb_define_method(cSubscrCreateParams, "port_number", subscr_cp_get_port_number, 0);
    rb_define_method(cSubscrCreateParams, "port_number=", subscr_cp_set_port_number, 1);
    rb_define_method(cSubscrCreateParams, "timeout", subscr_cp_get_timeout, 0);
    rb_define_method(cSubscrCreateParams, "timeout=", subscr_cp_set_timeout, 1);
    rb_define_method(cSubscrCreateParams, "name", subscr_cp_get_name, 0);
    rb_define_method(cSubscrCreateParams, "name=", subscr_cp_set_name, 1);
    rb_define_method(cSubscrCreateParams, "callback", subscr_cp_get_callback, 0);
    rb_define_method(cSubscrCreateParams, "callback=", subscr_cp_set_callback, 1);
    rb_define_method(cSubscrCreateParams, "recipient_name", subscr_cp_get_recipient_name, 0);
    rb_define_method(cSubscrCreateParams, "recipient_name=", subscr_cp_set_recipient_name, 1);

    /* Timestamp */
    cTimestamp = rb_define_class_under(mDpi, "Timestamp", rb_cObject);
    rb_define_alloc_func(cTimestamp, timestamp_alloc);
    rb_define_method(cTimestamp, "initialize", timestamp_initialize, 9);
    rb_define_method(cTimestamp, "initialize_copy", timestamp_initialize_copy, 1);
    rb_define_method(cTimestamp, "inspect", timestamp_inspect, 0);
    rb_define_method(cTimestamp, "year", timestamp_get_year, 0);
    rb_define_method(cTimestamp, "year=", timestamp_set_year, 1);
    rb_define_method(cTimestamp, "month", timestamp_get_month, 0);
    rb_define_method(cTimestamp, "month=", timestamp_set_month, 1);
    rb_define_method(cTimestamp, "day", timestamp_get_day, 0);
    rb_define_method(cTimestamp, "day=", timestamp_set_day, 1);
    rb_define_method(cTimestamp, "hour", timestamp_get_hour, 0);
    rb_define_method(cTimestamp, "hour=", timestamp_set_hour, 1);
    rb_define_method(cTimestamp, "minute", timestamp_get_minute, 0);
    rb_define_method(cTimestamp, "minute=", timestamp_set_minute, 1);
    rb_define_method(cTimestamp, "second", timestamp_get_second, 0);
    rb_define_method(cTimestamp, "second=", timestamp_set_second, 1);
    rb_define_method(cTimestamp, "fsecond", timestamp_get_fsecond, 0);
    rb_define_method(cTimestamp, "fsecond=", timestamp_set_fsecond, 1);
    rb_define_method(cTimestamp, "tz_hour_offset", timestamp_get_tz_hour_offset, 0);
    rb_define_method(cTimestamp, "tz_hour_offset=", timestamp_set_tz_hour_offset, 1);
    rb_define_method(cTimestamp, "tz_minute_offset", timestamp_get_tz_minute_offset, 0);
    rb_define_method(cTimestamp, "tz_minute_offset=", timestamp_set_tz_minute_offset, 1);
}

void rbdpi_copy_dpiConnCreateParams(VALUE dest, const dpiConnCreateParams *src)
{
    conn_cp_t *ccp = TO_CONN_CP(dest);

    if (ccp->params.outTag != NULL) {
        ccp->out_tag = rb_str_new(ccp->params.outTag, ccp->params.outTagLength);
    }
    ccp->gc_guard = Qnil;
}

void rbdpi_copy_dpiPoolCreateParams(VALUE dest, const dpiPoolCreateParams *src)
{
    pool_cp_t *pcp = TO_POOL_CP(dest);

    pcp->params = *src; /* TODO */
    if (pcp->params.outPoolName == NULL) {
        pcp->pool_name = Qnil;
    } else {
        pcp->pool_name = rb_str_new(pcp->params.outPoolName, pcp->params.outPoolNameLength);
    }
}

void rbdpi_fill_dpiCommonCreateParams(dpiCommonCreateParams *params, VALUE val)
{
    if (!NIL_P(val)) {
        common_cp_t *ccp = TO_COMMON_CP(val);

        *params = ccp->params;
        params->encoding = NSTR_PTR(ccp->encoding);
        params->nencoding = NSTR_PTR(ccp->nencoding);
        params->edition = NSTR_PTR(ccp->edition);
        params->editionLength = NSTR_LEN(ccp->edition);
        params->driverName = NSTR_PTR(ccp->driver_name);
        params->driverNameLength = NSTR_LEN(ccp->driver_name);
    } else {
        CHK(dpiContext_initCommonCreateParams(rbdpi_g_context, params));
        params->createMode = DPI_MODE_CREATE_THREADED;
        params->driverName = DEFAULT_DRIVER_NAME;
        params->driverNameLength = strlen(params->driverName);
    }
}

static VALUE make_dpiAppContext(dpiAppContext **out, uint32_t *out_len, VALUE app_context)
{
    long len, idx;
    VALUE gc_guard;
    VALUE obj;
    dpiAppContext *ac;

    Check_Type(app_context, T_ARRAY);
    len = RARRAY_LEN(app_context);
    gc_guard = rb_ary_new_capa(1 + 3 * len);
    obj = rb_str_tmp_new(len * sizeof(dpiAppContext));
    ac = (dpiAppContext *)RSTRING_PTR(obj);
    rb_ary_push(gc_guard, obj);
    for (idx = 0; idx < len; idx++) {
        VALUE elem = RARRAY_AREF(app_context, idx);

        Check_Type(elem, T_ARRAY);
        if (RARRAY_LEN(elem) != 3) {
            rb_raise(rb_eArgError, "invalid value for app context element");
        }

        obj = RARRAY_AREF(elem, 0);
        SafeStringValue(obj);
        ac[idx].namespaceName = RSTRING_PTR(obj);
        ac[idx].namespaceNameLength = RSTRING_LEN(obj);
        rb_ary_push(gc_guard, obj);

        obj = RARRAY_AREF(elem, 1);
        SafeStringValue(obj);
        ac[idx].name = RSTRING_PTR(obj);
        ac[idx].nameLength = RSTRING_LEN(obj);
        rb_ary_push(gc_guard, obj);

        obj = RARRAY_AREF(elem, 2);
        SafeStringValue(obj);
        ac[idx].value = RSTRING_PTR(obj);
        ac[idx].valueLength = RSTRING_LEN(obj);
        rb_ary_push(gc_guard, obj);
    }
    *out = ac;
    *out_len = len;
    return gc_guard;
}

void rbdpi_fill_dpiConnCreateParams(dpiConnCreateParams *params, VALUE val)
{
    if (!NIL_P(val)) {
        conn_cp_t *ccp = TO_CONN_CP(val);

        *params = ccp->params;
        params->connectionClass = NSTR_PTR(ccp->connection_class);
        params->connectionClassLength = NSTR_LEN(ccp->connection_class);
        params->newPassword = NSTR_PTR(ccp->new_password);
        params->newPasswordLength = NSTR_LEN(ccp->new_password);
        ccp->gc_guard = make_dpiAppContext(&params->appContext, &params->numAppContext, ccp->app_context);
        params->tag = NSTR_PTR(ccp->tag);
        params->tagLength = NSTR_LEN(ccp->tag);
        params->outTag = NULL;
        params->outTagLength = 0;
        params->outTagFound = 0;
    } else {
        CHK(dpiContext_initConnCreateParams(rbdpi_g_context, params));
    }
}

void rbdpi_fill_dpiPoolCreateParams(dpiPoolCreateParams *params, VALUE val)
{
    if (!NIL_P(val)) {
        *params = TO_POOL_CP(val)->params;
    } else {
        CHK(dpiContext_initPoolCreateParams(rbdpi_g_context, params));
    }
}

void rbdpi_fill_dpiSubscrCreateParams(dpiSubscrCreateParams *params, VALUE val)
{
    subscr_cp_t *scp = TO_SUBSCR_CP(val);

    *params = scp->params;
    params->name = NSTR_PTR(scp->name);
    params->nameLength = NSTR_LEN(scp->name);
    params->callback = NIL_P(scp->callback) ? NULL : (void*)scp->callback;
    params->callbackContext = NULL;
    params->recipientName = NSTR_PTR(scp->recipient_name);
    params->recipientNameLength = NSTR_LEN(scp->recipient_name);
}

VALUE rbdpi_from_dpiEncodingInfo(const dpiEncodingInfo *info)
{
    enc_info_t *ei;
    VALUE obj = TypedData_Make_Struct(cEncodingInfo, enc_info_t, &enc_info_data_type, ei);

    ei->encoding = rb_usascii_str_new_cstr(info->encoding);
    ei->nencoding = rb_usascii_str_new_cstr(info->nencoding);
    ei->maxBytesPerCharacter = info->maxBytesPerCharacter;
    ei->nmaxBytesPerCharacter = info->nmaxBytesPerCharacter;
    return obj;
}

VALUE rbdpi_from_dpiErrorInfo(const dpiErrorInfo *error)
{
    VALUE args[7];
    dpiErrorInfo errbuf;

    if (error == NULL) {
        dpiContext_getError(rbdpi_g_context, &errbuf);
        error = &errbuf;
    }

    args[0] = rb_external_str_new_with_enc(error->message, error->messageLength, rb_enc_find(error->encoding));
    args[1] = INT2NUM(error->code);
    args[2] = INT2FIX(error->offset);
    args[3] = rb_usascii_str_new_cstr(error->fnName);
    args[4] = rb_usascii_str_new_cstr(error->action);
    args[5] = rb_usascii_str_new_cstr(error->sqlState);
    args[6] = error->isRecoverable ? Qtrue : Qfalse;
    return rb_class_new_instance(7, args, cError);
}

VALUE rbdpi_from_dpiIntervalDS(const dpiIntervalDS *intvl)
{
    dpiIntervalDS *p;
    VALUE obj = TypedData_Make_Struct(cIntervalDS, dpiIntervalDS, &intvl_ds_data_type, p);

    *p = *intvl;
    return obj;
}

VALUE rbdpi_from_dpiIntervalYM(const dpiIntervalYM *intvl)
{
    dpiIntervalYM *p;
    VALUE obj = TypedData_Make_Struct(cIntervalYM, dpiIntervalYM, &intvl_ym_data_type, p);

    *p = *intvl;
    return obj;
}

VALUE rbdpi_from_dpiQueryInfo(const dpiQueryInfo *info, const rbdpi_enc_t *enc)
{
    VALUE obj = rb_obj_alloc(cQueryInfo);
    VALUE name = rb_external_str_new_with_enc(info->name, info->nameLength, enc->enc);
    VALUE datatype = rbdpi_from_dpiDataTypeInfo(&info->typeInfo, obj, enc);

    rb_ivar_set(obj, id_at_name, name);
    rb_ivar_set(obj, id_at_type_info, datatype);
    return obj;
}

VALUE rbdpi_from_dpiTimestamp(const dpiTimestamp *ts)
{
    dpiTimestamp *p;
    VALUE obj = TypedData_Make_Struct(cTimestamp, dpiTimestamp, &timestamp_data_type, p);

    *p = *ts;
    return obj;
}

void rbdpi_to_dpiIntervalDS(dpiIntervalDS *intvl, VALUE val)
{
    *intvl = *TO_INTVL_DS(val);
}

void rbdpi_to_dpiIntervalYM(dpiIntervalYM *intvl, VALUE val)
{
    *intvl = *TO_INTVL_YM(val);
}

void rbdpi_to_dpiTimestamp(dpiTimestamp *ts, VALUE val)
{
    *ts = *TO_TIMESTAMP(val);
}
