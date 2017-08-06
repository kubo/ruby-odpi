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

static VALUE pool_initialize(VALUE self, VALUE username, VALUE password, VALUE connect_string, VALUE common_params, VALUE create_params)
{
    pool_t *pool = rbdpi_to_pool(self);
    dpiCommonCreateParams commonParams;
    dpiPoolCreateParams createParams;

    rbdpi_fill_dpiCommonCreateParams(&commonParams, common_params);
    rbdpi_fill_dpiPoolCreateParams(&createParams, create_params);
    CHK(dpiPool_create_without_gvl(rbdpi_g_context,
                                   NIL_P(username) ? NULL : RSTRING_PTR(username),
                                   NIL_P(username) ? 0 : RSTRING_LEN(username),
                                   NIL_P(password) ? NULL : RSTRING_PTR(password),
                                   NIL_P(password) ? 0 : RSTRING_LEN(password),
                                   NIL_P(connect_string) ? NULL : RSTRING_PTR(connect_string),
                                   NIL_P(connect_string) ? 0 : RSTRING_LEN(connect_string),
                                   &commonParams, &createParams, &pool->handle));
    RB_GC_GUARD(username);
    RB_GC_GUARD(password);
    RB_GC_GUARD(connect_string);
    RB_GC_GUARD(common_params);
    if (!NIL_P(create_params)) {
        rbdpi_copy_dpiPoolCreateParams(create_params, &createParams);
    }
    return self;
}

static VALUE pool_connect(VALUE self, VALUE username, VALUE password, VALUE params)
{
    pool_t *pool = rbdpi_to_pool(self);
    dpiConn *conn;
    dpiConnCreateParams createParams;
    VALUE obj;

    rbdpi_fill_dpiConnCreateParams(&createParams, params);
    CHK(dpiPool_acquireConnection_without_gvl(pool->handle,
                                              NIL_P(username) ? NULL : RSTRING_PTR(username),
                                              NIL_P(username) ? 0 : RSTRING_LEN(username),
                                              NIL_P(password) ? NULL : RSTRING_PTR(password),
                                              NIL_P(password) ? 0 : RSTRING_LEN(password),
                                              &createParams, &conn));
    RB_GC_GUARD(username);
    RB_GC_GUARD(password);
    obj = rbdpi_from_conn(conn);
    if (!NIL_P(params)) {
        rbdpi_copy_dpiConnCreateParams(params, &createParams);
    }
    return obj;
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
    rb_define_method(cPool, "initialize", pool_initialize, 5);
    rb_define_method(cPool, "connect", pool_connect, 3);
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
