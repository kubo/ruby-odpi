/*
 * rbdpi-conn.c -- part of ruby-odpi
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

static VALUE cConn;
static VALUE sym_drop;

static void conn_free(void *arg)
{
    conn_t *conn = (conn_t *)arg;

    if (conn->handle != NULL) {
        dpiConn_release(conn->handle);
    }
    xfree(arg);
}

static const struct rb_data_type_struct conn_data_type = {
    "ODPI::Dpi::Conn",
    {NULL, conn_free, },
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE conn_alloc(VALUE klass)
{
    conn_t *conn;

    return TypedData_Make_Struct(klass, conn_t, &conn_data_type, conn);
}

static VALUE conn_initialize(VALUE self, VALUE username, VALUE password, VALUE connect_string, VALUE common_params, VALUE create_params)
{
    conn_t *conn = (conn_t *)rb_check_typeddata(self, &conn_data_type);
    dpiCommonCreateParams commonParams;
    dpiConnCreateParams createParams;
    dpiEncodingInfo encinfo;

    if (conn->handle != NULL) {
        rb_raise(rb_eRuntimeError, "Try to initialize an initialized connection");
    }

    CHK_NSTR(username);
    CHK_NSTR(password);
    CHK_NSTR(connect_string);
    rbdpi_fill_dpiCommonCreateParams(&commonParams, common_params);
    rbdpi_fill_dpiConnCreateParams(&createParams, create_params);
    CHK(dpiConn_create_without_gvl(rbdpi_g_context,
                                   NSTR_PTR(username), NSTR_LEN(username),
                                   NSTR_PTR(password), NSTR_LEN(password),
                                   NSTR_PTR(connect_string), NSTR_LEN(connect_string),
                                   &commonParams, &createParams, &conn->handle));
    RB_GC_GUARD(username);
    RB_GC_GUARD(password);
    RB_GC_GUARD(connect_string);
    RB_GC_GUARD(common_params);
    if (!NIL_P(create_params)) {
        rbdpi_copy_dpiConnCreateParams(create_params, &createParams);
    }
    CHK(dpiConn_getEncodingInfo(conn->handle, &encinfo));
    conn->enc.enc = rb_enc_find(encinfo.encoding);
    conn->enc.nenc = rb_enc_find(encinfo.nencoding);
    return self;
}

static VALUE conn_initialize_copy(VALUE self, VALUE other)
{
    conn_t *conn = rbdpi_to_conn(self);

    *conn = *rbdpi_to_conn(other);
    if (conn->handle != NULL) {
        CHK(dpiConn_addRef(conn->handle));
    }
    return self;
}

static VALUE conn_begin_distrib_trans(VALUE self, VALUE format_id, VALUE transaction_id, VALUE branch_id)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_STR_ENC(transaction_id, conn->enc.enc);
    CHK_STR_ENC(branch_id, conn->enc.enc);
    CHK(dpiConn_beginDistribTrans_without_gvl(conn->handle, NUM2LONG(format_id),
                                              RSTRING_PTR(transaction_id),
                                              RSTRING_LEN(transaction_id),
                                              RSTRING_PTR(branch_id),
                                              RSTRING_LEN(branch_id)));
    RB_GC_GUARD(transaction_id);
    RB_GC_GUARD(branch_id);
    return self;
}

static VALUE conn_break(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK(dpiConn_breakExecution_without_gvl(conn->handle));
    return self;
}

static VALUE conn_change_password(VALUE self, VALUE user_name, VALUE old_password, VALUE new_password)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_STR_ENC(user_name, conn->enc.enc);
    CHK_STR_ENC(old_password, conn->enc.enc);
    CHK_STR_ENC(new_password, conn->enc.enc);

    CHK(dpiConn_changePassword_without_gvl(conn->handle,
                                           RSTRING_PTR(user_name),
                                           RSTRING_LEN(user_name),
                                           RSTRING_PTR(old_password),
                                           RSTRING_LEN(old_password),
                                           RSTRING_PTR(new_password),
                                           RSTRING_LEN(new_password)));
    RB_GC_GUARD(user_name);
    RB_GC_GUARD(old_password);
    RB_GC_GUARD(new_password);
    return self;
}

static VALUE conn_close(VALUE self, VALUE mode, VALUE tag)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_NSTR(tag);
    CHK(dpiConn_close_without_gvl(conn->handle,
                                  rbdpi_to_dpiConnCloseMode(mode),
                                  NSTR_PTR(tag), NSTR_LEN(tag)));
    RB_GC_GUARD(tag);
    return self;
}

static VALUE conn_commit(VALUE self)
{
    CHK(dpiConn_commit_without_gvl(rbdpi_to_conn(self)->handle));
    return self;
}

static VALUE conn_deq_object(VALUE self, VALUE queue_name, VALUE options, VALUE props, VALUE payload)
{
    conn_t *conn = rbdpi_to_conn(self);
    const char *msgId;
    uint32_t msgIdLength;

    CHK_STR_ENC(queue_name, conn->enc.enc);
    CHK(dpiConn_deqObject_without_gvl(conn->handle, RSTRING_PTR(queue_name),
                                      RSTRING_LEN(queue_name),
                                      rbdpi_to_deq_options(options)->handle,
                                      rbdpi_to_msg_props(props)->handle,
                                      rbdpi_to_object(payload)->handle,
                                      &msgId, &msgIdLength));
    RB_GC_GUARD(queue_name);
    return STR_NEW_ENC(msgId, msgIdLength, conn->enc.enc);
}

static VALUE conn_enq_object(VALUE self, VALUE queue_name, VALUE options, VALUE props, VALUE payload)
{
    conn_t *conn = rbdpi_to_conn(self);
    const char *msgId;
    uint32_t msgIdLength;

    CHK_STR_ENC(queue_name, conn->enc.enc);
    CHK(dpiConn_enqObject_without_gvl(conn->handle, RSTRING_PTR(queue_name),
                                      RSTRING_LEN(queue_name),
                                      rbdpi_to_enq_options(options)->handle,
                                      rbdpi_to_msg_props(props)->handle,
                                      rbdpi_to_object(payload)->handle,
                                      &msgId, &msgIdLength));
    RB_GC_GUARD(queue_name);
    return STR_NEW_ENC(msgId, msgIdLength, conn->enc.enc);
}

static VALUE conn_get_current_schema(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    const char *val;
    uint32_t len;

    CHK(dpiConn_getCurrentSchema(conn->handle, &val, &len));
    return NSTR_NEW_ENC(val, len, conn->enc.enc);
}

static VALUE conn_get_edition(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    const char *val;
    uint32_t len;

    CHK(dpiConn_getEdition(conn->handle, &val, &len));
    return NSTR_NEW_ENC(val, len, conn->enc.enc);
}

static VALUE conn_get_encoding_info(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    dpiEncodingInfo info;

    CHK(dpiConn_getEncodingInfo(conn->handle, &info));
    return rbdpi_from_dpiEncodingInfo(&info);
}

static VALUE conn_get_external_name(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    const char *val;
    uint32_t len;

    CHK(dpiConn_getExternalName(conn->handle, &val, &len));
    return NSTR_NEW_ENC(val, len, conn->enc.enc);
}

static VALUE conn_get_internal_name(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    const char *val;
    uint32_t len;

    CHK(dpiConn_getInternalName(conn->handle, &val, &len));
    return NSTR_NEW_ENC(val, len, conn->enc.enc);
}

static VALUE conn_get_ltxid(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    const char *val;
    uint32_t len;

    CHK(dpiConn_getLTXID(conn->handle, &val, &len));
    return NSTR_NEW_ENC(val, len, conn->enc.enc);
}

static VALUE conn_get_object_type(VALUE self, VALUE name)
{
    conn_t *conn = rbdpi_to_conn(self);
    dpiObjectType *handle;

    CHK_STR_ENC(name, conn->enc.enc);
    CHK(dpiConn_getObjectType_without_gvl(conn->handle, RSTRING_PTR(name), RSTRING_LEN(name), &handle));
    RB_GC_GUARD(name);
    return rbdpi_from_object_type(handle, &conn->enc);
}

static VALUE conn_get_server_release(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    const char *str;
    uint32_t len;
    dpiVersionInfo ver;

    CHK(dpiConn_getServerVersion(conn->handle, &str, &len, &ver));
    return rb_usascii_str_new(str, len);
}

static VALUE conn_get_server_version(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    const char *str;
    uint32_t len;
    dpiVersionInfo ver;

    CHK(dpiConn_getServerVersion(conn->handle, &str, &len, &ver));
    return rbdpi_from_dpiVersionInfo(&ver);
}

static VALUE conn_get_stmt_cache_size(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    uint32_t size;

    CHK(dpiConn_getStmtCacheSize(conn->handle, &size));
    return ULONG2NUM(size);
}

static VALUE conn_new_deq_options(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    dpiDeqOptions *options;

    CHK(dpiConn_newDeqOptions(conn->handle, &options));
    return rbdpi_from_deq_options(options, conn->enc.enc);
}

static VALUE conn_new_enq_options(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    dpiEnqOptions *options;

    CHK(dpiConn_newEnqOptions(conn->handle, &options));
    return rbdpi_from_enq_options(options, conn->enc.enc);
}

static VALUE conn_new_msg_props(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    dpiMsgProps *props;

    CHK(dpiConn_newMsgProps(conn->handle, &props));
    return rbdpi_from_msg_props(props, conn->enc.enc);
}

static VALUE conn_new_subscription(VALUE self, VALUE params)
{
    conn_t *conn = rbdpi_to_conn(self);
    dpiSubscrCreateParams create_params;
    dpiSubscr *handle;
    subscr_t *subscr;
    VALUE obj;

    rbdpi_fill_dpiSubscrCreateParams(&create_params, params);
    obj = rbdpi_subscr_prepare(&subscr, &create_params, &conn->enc);
    CHK(dpiConn_newSubscription(conn->handle, &create_params, &handle, NULL));
    RB_GC_GUARD(params);
    subscr->handle = handle;
    rbdpi_subscr_start(subscr);
    return obj;
}

static VALUE conn_new_temp_lob(VALUE self, VALUE lobtype)
{
    conn_t *conn = rbdpi_to_conn(self);
    dpiOracleTypeNum lobtype_num = rbdpi_to_dpiOracleTypeNum(lobtype);
    dpiLob *lob;

    CHK(dpiConn_newTempLob(conn->handle, lobtype_num, &lob));
    return rbdpi_from_lob(lob, &conn->enc, lobtype_num);
}

static VALUE conn_new_var(VALUE self, VALUE oracle_type, VALUE native_type,
                          VALUE max_array_size, VALUE size, VALUE size_is_bytes,
                          VALUE is_array, VALUE objtype)
{
    conn_t *conn = rbdpi_to_conn(self);
    dpiOracleTypeNum oracle_type_num = rbdpi_to_dpiOracleTypeNum(oracle_type);
    dpiNativeTypeNum native_type_num = rbdpi_to_dpiNativeTypeNum(native_type);
    dpiVar *var;
    dpiData *data;

    CHK(dpiConn_newVar(conn->handle, oracle_type_num, native_type_num,
                       NUM2UINT(max_array_size), NUM2UINT(size), RTEST(size_is_bytes),
                       RTEST(is_array), NIL_P(objtype) ? NULL : rbdpi_to_object_type(objtype)->handle,
                       &var, &data));
    return rbdpi_from_var(var, &conn->enc, oracle_type_num, native_type_num, objtype);
}

static VALUE conn_ping(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK(dpiConn_ping(conn->handle));
    return self;
}

static VALUE conn_prepare_distrib_trans(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);
    int commit_needed;

    CHK(dpiConn_prepareDistribTrans(conn->handle, &commit_needed));
    return commit_needed ? Qtrue : Qfalse;
}

static VALUE conn_prepare_stmt(VALUE self, VALUE scrollable, VALUE sql, VALUE tag)
{
    conn_t *conn = rbdpi_to_conn(self);
    dpiStmt *stmt;

    CHK_STR_ENC(sql, conn->enc.enc);
    CHK_NSTR_ENC(tag, conn->enc.enc);
    CHK(dpiConn_prepareStmt(conn->handle, RTEST(scrollable),
                            RSTRING_PTR(sql), RSTRING_LEN(sql),
                            NSTR_PTR(tag), NSTR_LEN(tag),
                            &stmt));
    RB_GC_GUARD(sql);
    RB_GC_GUARD(tag);
    return rbdpi_from_stmt(stmt, &conn->enc);
}

static VALUE conn_rollback(VALUE self)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK(dpiConn_rollback(conn->handle));
    return self;
}

static VALUE conn_set_action(VALUE self, VALUE val)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_NSTR_ENC(val, conn->enc.enc);
    CHK(dpiConn_setAction(conn->handle, NSTR_PTR(val), NSTR_LEN(val)));
    return val;
}

static VALUE conn_set_client_identifier(VALUE self, VALUE val)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_NSTR_ENC(val, conn->enc.enc);
    CHK(dpiConn_setClientIdentifier(conn->handle, NSTR_PTR(val), NSTR_LEN(val)));
    return val;
}

static VALUE conn_set_client_info(VALUE self, VALUE val)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_NSTR_ENC(val, conn->enc.enc);
    CHK(dpiConn_setClientInfo(conn->handle, NSTR_PTR(val), NSTR_LEN(val)));
    return val;
}

static VALUE conn_set_current_schema(VALUE self, VALUE val)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_STR_ENC(val, conn->enc.enc);
    CHK(dpiConn_setCurrentSchema(conn->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    return val;
}

static VALUE conn_set_dbop(VALUE self, VALUE val)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_NSTR_ENC(val, conn->enc.enc);
    CHK(dpiConn_setDbOp(conn->handle, NSTR_PTR(val), NSTR_LEN(val)));
    return val;
}

static VALUE conn_set_external_name(VALUE self, VALUE val)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_NSTR_ENC(val, conn->enc.enc);
    CHK(dpiConn_setExternalName(conn->handle, NSTR_PTR(val), NSTR_LEN(val)));
    return val;
}

static VALUE conn_set_internal_name(VALUE self, VALUE val)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_NSTR_ENC(val, conn->enc.enc);
    CHK(dpiConn_setInternalName(conn->handle, NSTR_PTR(val), NSTR_LEN(val)));
    return val;
}

static VALUE conn_set_module(VALUE self, VALUE val)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK_NSTR_ENC(val, conn->enc.enc);
    CHK(dpiConn_setModule(conn->handle, NSTR_PTR(val), NSTR_LEN(val)));
    return val;
}

static VALUE conn_set_stmt_cache_size(VALUE self, VALUE val)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK(dpiConn_setStmtCacheSize(conn->handle, NUM2UINT(val)));
    return val;
}

static VALUE conn_shutdown_database(VALUE self, VALUE mode)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK(dpiConn_shutdownDatabase(conn->handle, rbdpi_to_dpiShutdownMode(mode)));
    return self;
}

static VALUE conn_startup_database(VALUE self, VALUE mode)
{
    conn_t *conn = rbdpi_to_conn(self);

    CHK(dpiConn_startupDatabase(conn->handle, rbdpi_to_dpiStartupMode(mode)));
    return self;
}

void Init_rbdpi_conn(VALUE mDpi)
{
    sym_drop = ID2SYM(rb_intern("drop"));

    cConn = rb_define_class_under(mDpi, "Conn", rb_cObject);
    rb_define_alloc_func(cConn, conn_alloc);

    rb_define_method(cConn, "initialize", conn_initialize, 5);
    rb_define_method(cConn, "initialize_copy", conn_initialize_copy, 1);
    rb_define_method(cConn, "begin_distrib_trans", conn_begin_distrib_trans, 3);
    rb_define_method(cConn, "break", conn_break, 0);
    rb_define_method(cConn, "change_password", conn_change_password, 3);
    rb_define_method(cConn, "close", conn_close, 2);
    rb_define_method(cConn, "commit", conn_commit, 0);
    rb_define_method(cConn, "deq_object", conn_deq_object, 4);
    rb_define_method(cConn, "enq_object", conn_enq_object, 4);
    rb_define_method(cConn, "current_schema", conn_get_current_schema, 0);
    rb_define_method(cConn, "edition", conn_get_edition, 0);
    rb_define_method(cConn, "encoding_info", conn_get_encoding_info, 0);
    rb_define_method(cConn, "external_name", conn_get_external_name, 0);
    rb_define_method(cConn, "internal_name", conn_get_internal_name, 0);
    rb_define_method(cConn, "ltxid", conn_get_ltxid, 0);
    rb_define_method(cConn, "object_type", conn_get_object_type, 1);
    rb_define_method(cConn, "server_release", conn_get_server_release, 0);
    rb_define_method(cConn, "server_version", conn_get_server_version, 0);
    rb_define_method(cConn, "stmt_cache_size", conn_get_stmt_cache_size, 0);
    rb_define_method(cConn, "new_deq_options", conn_new_deq_options, 0);
    rb_define_method(cConn, "new_enq_options", conn_new_enq_options, 0);
    rb_define_method(cConn, "new_msg_props", conn_new_msg_props, 0);
    rb_define_method(cConn, "new_subscription", conn_new_subscription, 1);
    rb_define_method(cConn, "new_temp_lob", conn_new_temp_lob, 1);
    rb_define_method(cConn, "new_var", conn_new_var, 7);
    rb_define_method(cConn, "ping", conn_ping, 0);
    rb_define_method(cConn, "prepare_distrib_trans", conn_prepare_distrib_trans, 0);
    rb_define_method(cConn, "prepare_stmt", conn_prepare_stmt, 3);
    rb_define_method(cConn, "rollback", conn_rollback, 0);
    rb_define_method(cConn, "action=", conn_set_action, 1);
    rb_define_method(cConn, "client_identifier=", conn_set_client_identifier, 1);
    rb_define_method(cConn, "client_info=", conn_set_client_info, 1);
    rb_define_method(cConn, "current_schema=", conn_set_current_schema, 1);
    rb_define_method(cConn, "dbop=", conn_set_dbop, 1);
    rb_define_method(cConn, "external_name=", conn_set_external_name, 1);
    rb_define_method(cConn, "internal_name=", conn_set_internal_name, 1);
    rb_define_method(cConn, "module=", conn_set_module, 1);
    rb_define_method(cConn, "stmt_cache_size=", conn_set_stmt_cache_size, 1);
    rb_define_method(cConn, "shutdown_database", conn_shutdown_database, 1);
    rb_define_method(cConn, "startup_database", conn_startup_database, 1);
}

VALUE rbdpi_from_conn(dpiConn *handle)
{
    conn_t *conn;
    VALUE obj = TypedData_Make_Struct(cConn, conn_t, &conn_data_type, conn);
    dpiEncodingInfo encinfo;

    conn->handle = handle;
    CHK(dpiConn_getEncodingInfo(conn->handle, &encinfo));
    conn->enc.enc = rb_enc_find(encinfo.encoding);
    conn->enc.nenc = rb_enc_find(encinfo.nencoding);
    return obj;
}

conn_t *rbdpi_to_conn(VALUE obj)
{
    conn_t *conn = (conn_t *)rb_check_typeddata(obj, &conn_data_type);

    if (conn->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return conn;
}
