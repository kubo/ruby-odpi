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

static VALUE cEncodingInfo;
static VALUE cError;
static VALUE cIntervalDS;
static VALUE cIntervalYM;
static VALUE cQueryInfo;
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
    id_at_name = rb_intern("@name");
    id_at_type_info = rb_intern("@type_info");

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

    /* QueryInfo */
    cQueryInfo = rb_define_class_under(mDpi, "QueryInfo", rb_cObject);
    rb_define_method(cQueryInfo, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cQueryInfo, "inspect", query_info_inspect, 0);
    rb_define_method(cQueryInfo, "name", query_info_get_name, 0);
    rb_define_method(cQueryInfo, "type_info", query_info_get_type_info, 0);

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
