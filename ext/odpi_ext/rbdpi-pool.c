/*
 * rbdpi-pool.c -- part of ruby-odpi
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

static void pool_free(void *arg)
{
    dpiPool_release(((pool_t*)arg)->handle);
    xfree(arg);
}

static const struct rb_data_type_struct pool_data_type = {
    "ODPI::Dpi::Pool",
    {NULL, pool_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE pool_alloc(VALUE klass)
{
    pool_t *pool;

    return TypedData_Make_Struct(klass, pool_t, &pool_data_type, pool);
}

static VALUE pool_initialize(VALUE self, VALUE username, VALUE password, VALUE database, VALUE params)
{
    pool_t *pool = (pool_t *)rb_check_typeddata(self, &pool_data_type);
    dpiCommonCreateParams common_params;
    dpiPoolCreateParams create_params;
    VALUE gc_guard1;
    VALUE gc_guard2;

    if (pool->handle != NULL) {
        rb_raise(rb_eRuntimeError, "Try to initialize an initialized connection");
    }
    pool->enc = rbdpi_get_encodings(params);
    CHK_NSTR_ENC(username, pool->enc.enc);
    CHK_NSTR_ENC(password, pool->enc.enc);
    CHK_NSTR_ENC(database, pool->enc.enc);
    gc_guard1 = rbdpi_fill_dpiCommonCreateParams(&common_params, params, pool->enc.enc);
    gc_guard2 = rbdpi_fill_dpiPoolCreateParams(&create_params, params, pool->enc.enc);
    if (NIL_P(username) && NIL_P(password)) {
        create_params.externalAuth = 1;
    }
    CHK(dpiPool_create_without_gvl(rbdpi_g_context,
                                   NSTR_PTR(username), NSTR_LEN(username),
                                   NSTR_PTR(password), NSTR_LEN(password),
                                   NSTR_PTR(database), NSTR_LEN(database),
                                   &common_params, &create_params, &pool->handle));
    RB_GC_GUARD(username);
    RB_GC_GUARD(password);
    RB_GC_GUARD(database);
    RB_GC_GUARD(gc_guard1);
    RB_GC_GUARD(gc_guard2);
    return self;
}

static VALUE pool_get_connection(VALUE self, VALUE username, VALUE password, VALUE auth_mode, VALUE params)
{
    pool_t *pool = rbdpi_to_pool(self);
    dpiConn *conn;
    dpiConnCreateParams create_params;
    VALUE gc_guard;

    CHK_NSTR_ENC(username, pool->enc.enc);
    CHK_NSTR_ENC(password, pool->enc.enc);
    gc_guard = rbdpi_fill_dpiConnCreateParams(&create_params, params, auth_mode, pool->enc.enc);
    if (NIL_P(username) && NIL_P(password)) {
        create_params.externalAuth = 1;
    }
    CHK(dpiPool_acquireConnection_without_gvl(pool->handle,
                                              NSTR_PTR(username), NSTR_LEN(username),
                                              NSTR_PTR(password), NSTR_LEN(password),
                                              &create_params, &conn));
    RB_GC_GUARD(username);
    RB_GC_GUARD(password);
    RB_GC_GUARD(gc_guard);
    return rbdpi_from_conn(conn, &create_params, &pool->enc);
}

static VALUE pool_close(VALUE self, VALUE mode)
{
    pool_t *pool = rbdpi_to_pool(self);

    CHK(dpiPool_close_without_gvl(pool->handle, rbdpi_to_dpiPoolCloseMode(mode)));
    return self;
}

static VALUE pool_get_busy_count(VALUE self)
{
    pool_t *pool = rbdpi_to_pool(self);
    uint32_t val;

    CHK(dpiPool_getBusyCount(pool->handle, &val));
    return UINT2NUM(val);
}

static VALUE pool_get_encoding_info(VALUE self)
{
    pool_t *pool = rbdpi_to_pool(self);
    dpiEncodingInfo info;

    CHK(dpiPool_getEncodingInfo(pool->handle, &info));
    return rbdpi_from_dpiEncodingInfo(&info);
}

static VALUE pool_get_get_mode(VALUE self)
{
    pool_t *pool = rbdpi_to_pool(self);
    dpiPoolGetMode mode;

    CHK(dpiPool_getGetMode(pool->handle, &mode));
    return rbdpi_from_dpiPoolGetMode(mode);
}

static VALUE pool_get_max_lifetime_session(VALUE self)
{
    pool_t *pool = rbdpi_to_pool(self);
    uint32_t val;

    CHK(dpiPool_getMaxLifetimeSession(pool->handle, &val));
    return UINT2NUM(val);
}

static VALUE pool_get_open_count(VALUE self)
{
    pool_t *pool = rbdpi_to_pool(self);
    uint32_t val;

    CHK(dpiPool_getOpenCount(pool->handle, &val));
    return UINT2NUM(val);
}

static VALUE pool_get_stmt_cache_size(VALUE self)
{
    pool_t *pool = rbdpi_to_pool(self);
    uint32_t val;

    CHK(dpiPool_getStmtCacheSize(pool->handle, &val));
    return UINT2NUM(val);
}

static VALUE pool_get_timeout(VALUE self)
{
    pool_t *pool = rbdpi_to_pool(self);
    uint32_t val;

    CHK(dpiPool_getTimeout(pool->handle, &val));
    return UINT2NUM(val);
}

static VALUE pool_set_get_mode(VALUE self, VALUE val)
{
    pool_t *pool = rbdpi_to_pool(self);

    CHK(dpiPool_setGetMode(pool->handle, rbdpi_to_dpiPoolGetMode(val)));
    return val;
}

static VALUE pool_set_max_lifetime_session(VALUE self, VALUE val)
{
    pool_t *pool = rbdpi_to_pool(self);

    CHK(dpiPool_setMaxLifetimeSession(pool->handle, NUM2UINT(val)));
    return val;
}

static VALUE pool_set_stmt_cache_size(VALUE self, VALUE val)
{
    pool_t *pool = rbdpi_to_pool(self);

    CHK(dpiPool_setStmtCacheSize(pool->handle, NUM2UINT(val)));
    return val;
}

static VALUE pool_set_timeout(VALUE self, VALUE val)
{
    pool_t *pool = rbdpi_to_pool(self);

    CHK(dpiPool_setTimeout(pool->handle, NUM2UINT(val)));
    return val;
}

void Init_rbdpi_pool(VALUE mDpi)
{
    VALUE cPool = rb_define_class_under(mDpi, "Pool", rb_cObject);
    rb_define_alloc_func(cPool, pool_alloc);
    rb_define_method(cPool, "initialize", pool_initialize, 4);
    rb_define_method(cPool, "connection", pool_get_connection, 4);
    rb_define_method(cPool, "close", pool_close, 1);
    rb_define_method(cPool, "busy_count", pool_get_busy_count, 0);
    rb_define_method(cPool, "encoding_info", pool_get_encoding_info, 0);
    rb_define_method(cPool, "get_mode", pool_get_get_mode, 0);
    rb_define_method(cPool, "max_lifetime_session", pool_get_max_lifetime_session, 0);
    rb_define_method(cPool, "open_count", pool_get_open_count, 0);
    rb_define_method(cPool, "stmt_cache_size", pool_get_stmt_cache_size, 0);
    rb_define_method(cPool, "timeout", pool_get_timeout, 0);
    rb_define_method(cPool, "get_mode=", pool_set_get_mode, 1);
    rb_define_method(cPool, "max_lifetime_session=", pool_set_max_lifetime_session, 0);
    rb_define_method(cPool, "stmt_cache_size=", pool_set_stmt_cache_size, 0);
    rb_define_method(cPool, "timeout=", pool_set_timeout, 0);
}

pool_t *rbdpi_to_pool(VALUE obj)
{
    pool_t *pool = (pool_t *)rb_check_typeddata(obj, &pool_data_type);

    if (pool->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return pool;
}
