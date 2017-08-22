/*
 * rbdpi.h -- part of ruby-odpi
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
#ifndef RBDPI_H
#define RBDPI_H 1
#include <ruby.h>
#include <ruby/encoding.h>
#include <dpi.h>
#include "rbdpi-func.h"

#define DEFAULT_DRIVER_NAME "ruby-odpi : " RBODPI_VERSION

#ifdef WIN32
#define mutex_t CRITICAL_SECTION
#define mutex_init(mutex) InitializeCriticalSection(&mutex)
#define mutex_destroy(mutex) DeleteCriticalSection(&mutex)
#define mutex_lock(mutex) EnterCriticalSection(&mutex)
#define mutex_unlock(mutex) LeaveCriticalSection(&mutex)
#else
#include "pthread.h"
#define mutex_t pthread_mutex_t
#define mutex_init(mutex) pthread_mutex_init(mutex, NULL)
#define mutex_destroy(mutex) pthread_mutex_destroy(mutex)
#define mutex_lock(mutex) pthread_mutex_lock(mutex)
#define mutex_unlock(mutex) pthread_mutex_unlock(mutex)
#endif

typedef struct {
    const rb_encoding *enc;  /* CHAR encoding */
    const rb_encoding *nenc; /* NCHAR encoding */
} rbdpi_enc_t;

typedef struct subscr_callback_ctx subscr_callback_ctx_t;

typedef struct {
    dpiConn *handle;
    rbdpi_enc_t enc;
    VALUE tag;
} conn_t;

/* ODPI::Dpi::DataType
 *
 * Don't access info.objectType.
 * It must be accessed via the objtype member.
 */
typedef struct {
    const dpiDataTypeInfo *info;
    VALUE owner;
    VALUE self;
    VALUE objtype; /* ODPI::Dpi::ObjectType or nil */
    rbdpi_enc_t enc;
} data_type_t;

typedef struct {
    dpiDeqOptions *handle;
    rb_encoding *enc;
} deq_options_t;

typedef struct {
    dpiEnqOptions *handle;
    rb_encoding *enc;
} enq_options_t;

typedef struct {
    dpiLob *handle;
    rbdpi_enc_t enc;
    dpiOracleTypeNum oracle_type;
} lob_t;

typedef struct {
    dpiMsgProps *handle;
    rb_encoding *enc;
} msg_props_t;

typedef struct {
    dpiObjectAttr *handle;
    rbdpi_enc_t enc;
    dpiObjectAttrInfo info;
    VALUE datatype;
} object_attr_t;

typedef struct {
    dpiObjectType *handle;
    rbdpi_enc_t enc;
    dpiObjectTypeInfo info;
    VALUE elem_datatype;
} object_type_t;

typedef struct {
    dpiObject *handle;
    VALUE objtype;
    object_type_t *object_type;
} object_t;

typedef struct {
    dpiPool *handle;
    rbdpi_enc_t enc;
} pool_t;

typedef struct {
    dpiStmt *handle;
    rbdpi_enc_t enc;
    dpiStmtInfo info;
    VALUE query_columns_cache;
} stmt_t;

typedef struct {
    dpiRowid *handle;
} rowid_t;

typedef struct {
    dpiSubscr *handle;
    rbdpi_enc_t enc;
    subscr_callback_ctx_t *callback_ctx;
} subscr_t;

typedef struct {
    dpiVar *handle;
    rbdpi_enc_t enc;
    dpiOracleTypeNum oracle_type;
    dpiNativeTypeNum native_type;
    VALUE objtype;
} var_t;

#define rbdpi_raise_error(error) rb_exc_raise(rbdpi_from_dpiErrorInfo(error))

/* Check whether nil or safe string */
#define CHK_NSTR(v) do { \
    if (!NIL_P(v)) { \
        SafeStringValue(v); \
    } \
} while (0)

/* Check whether safe string and covert encoding */
#define CHK_STR_ENC(v, enc) do { \
    SafeStringValue(v); \
    (v) = rb_str_export_to_enc(v, enc); \
} while (0)

/* Check whether nil or safe string and covert encoding */
#define CHK_NSTR_ENC(v, enc) do { \
    if (!NIL_P(v)) { \
        CHK_STR_ENC(v, enc); \
    } \
} while (0)

#define NSTR_PTR(v) (!NIL_P(v) ? RSTRING_PTR(v) : NULL)
#define NSTR_LEN(v) (!NIL_P(v) ? RSTRING_LEN(v) : 0)
#define NSTR_LEN_PTR_PAIR(obj) (int)NSTR_LEN(obj), NSTR_PTR(obj)
#define STR_NEW_ENC(v, len, enc) rb_external_str_new_with_enc((v), (len), (enc));
#define NSTR_NEW_ENC(v, len, enc) ((len) ? rb_external_str_new_with_enc((v), (len), (enc)) : Qnil)

#define CHK(func) do { \
    if ((func) != DPI_SUCCESS) { \
        rbdpi_raise_error(NULL); \
    } \
} while (0)

enum enc_type {
    ENC_TYPE_CHAR,
    ENC_TYPE_NCHAR,
    ENC_TYPE_OTHER,
};

static inline enum enc_type rbdpi_ora2enc_type(dpiOracleTypeNum oratype)
{
    switch (oratype) {
    case DPI_ORACLE_TYPE_VARCHAR:
    case DPI_ORACLE_TYPE_CHAR:
    case DPI_ORACLE_TYPE_CLOB:
    case DPI_ORACLE_TYPE_LONG_VARCHAR:
        return ENC_TYPE_CHAR;
    case DPI_ORACLE_TYPE_NVARCHAR:
    case DPI_ORACLE_TYPE_NCHAR:
    case DPI_ORACLE_TYPE_NCLOB:
        return ENC_TYPE_NCHAR;
    default:
        return ENC_TYPE_OTHER;
    }
}

/* rbdpi.c */
extern const dpiContext *rbdpi_g_context;
extern VALUE rbdpi_sym_encoding;
extern VALUE rbdpi_sym_nencoding;
VALUE rbdpi_initialize_error(VALUE self);

/* rbdpi-conn.c */
void Init_rbdpi_conn(VALUE mDpi);
VALUE rbdpi_from_conn(dpiConn *conn, dpiConnCreateParams *params, rbdpi_enc_t *enc);
conn_t *rbdpi_to_conn(VALUE obj);

/* rbdpi-create-params.c */
void Init_rbdpi_create_params(VALUE mODPI);
rbdpi_enc_t rbdpi_get_encodings(VALUE params);
VALUE rbdpi_fill_dpiCommonCreateParams(dpiCommonCreateParams *dpi_params, VALUE params, rb_encoding *enc);
VALUE rbdpi_fill_dpiConnCreateParams(dpiConnCreateParams *dpi_params, VALUE params, VALUE auth_mode, rb_encoding *enc);
VALUE rbdpi_fill_dpiPoolCreateParams(dpiPoolCreateParams *dpi_params, VALUE params, rb_encoding *enc);
VALUE rbdpi_fill_dpiSubscrCreateParams(dpiSubscrCreateParams *dpi_params, VALUE params, rb_encoding *enc);

/* rbdpi-data.c */
VALUE rbdpi_from_dpiData(const dpiData *data, dpiNativeTypeNum type, VALUE datatype);
VALUE rbdpi_from_dpiData2(const dpiData *data, dpiNativeTypeNum type, const rbdpi_enc_t *enc, dpiOracleTypeNum oratype, VALUE objtype);
VALUE rbdpi_to_dpiData(dpiData *data, VALUE val, dpiNativeTypeNum type, VALUE datatype);
VALUE rbdpi_to_dpiData2(dpiData *data, VALUE val, dpiNativeTypeNum type, const rbdpi_enc_t *enc, dpiOracleTypeNum oratype, VALUE objtype);

/* rbdpi-data-type.c */
void Init_rbdpi_data_type(VALUE mDpi);
VALUE rbdpi_from_dpiDataTypeInfo(const dpiDataTypeInfo *info, VALUE owner, const rbdpi_enc_t *enc);
const data_type_t *rbdpi_to_data_type(VALUE obj);

/* rbdpi-deq-options.c */
void Init_rbdpi_deq_options(VALUE mDpi);
VALUE rbdpi_from_deq_options(dpiDeqOptions *options, rb_encoding *enc);
deq_options_t *rbdpi_to_deq_options(VALUE obj);

/* rbdpi-enq-options.c */
void Init_rbdpi_enq_options(VALUE mDpi);
VALUE rbdpi_from_enq_options(dpiEnqOptions *options, rb_encoding *enc);
enq_options_t *rbdpi_to_enq_options(VALUE obj);

/* rbdpi-enum.c */
void Init_rbdpi_enum(void);
VALUE rbdpi_from_dpiAuthMode(dpiAuthMode val);
VALUE rbdpi_from_dpiCreateMode(dpiCreateMode val);
VALUE rbdpi_from_dpiDeqMode(dpiDeqMode val);
VALUE rbdpi_from_dpiDeqNavigation(dpiDeqNavigation val);
VALUE rbdpi_from_dpiEventType(dpiEventType val);
VALUE rbdpi_from_dpiMessageDeliveryMode(dpiMessageDeliveryMode val);
VALUE rbdpi_from_dpiMessageState(dpiMessageState val);
VALUE rbdpi_from_dpiNativeTypeNum(dpiNativeTypeNum val);
VALUE rbdpi_from_dpiOpCode(dpiOpCode val);
VALUE rbdpi_from_dpiOracleTypeNum(dpiOracleTypeNum val);
VALUE rbdpi_from_dpiPoolGetMode(dpiPoolGetMode val);
VALUE rbdpi_from_dpiPurity(dpiPurity val);
VALUE rbdpi_from_dpiStatementType(dpiStatementType val);
VALUE rbdpi_from_dpiSubscrNamespace(dpiSubscrNamespace val);
VALUE rbdpi_from_dpiSubscrProtocol(dpiSubscrProtocol val);
VALUE rbdpi_from_dpiSubscrQOS(dpiSubscrQOS val);
VALUE rbdpi_from_dpiVisibility(dpiVisibility val);
dpiAuthMode rbdpi_to_dpiAuthMode(VALUE val);
dpiConnCloseMode rbdpi_to_dpiConnCloseMode(VALUE val);
dpiDeqMode rbdpi_to_dpiDeqMode(VALUE val);
dpiDeqNavigation rbdpi_to_dpiDeqNavigation(VALUE val);
dpiExecMode rbdpi_to_dpiExecMode(VALUE val);
dpiFetchMode rbdpi_to_dpiFetchMode(VALUE val);
dpiMessageDeliveryMode rbdpi_to_dpiMessageDeliveryMode(VALUE val);
dpiNativeTypeNum rbdpi_to_dpiNativeTypeNum(VALUE val);
dpiOpCode rbdpi_to_dpiOpCode(VALUE val);
dpiOracleTypeNum rbdpi_to_dpiOracleTypeNum(VALUE val);
dpiPoolCloseMode rbdpi_to_dpiPoolCloseMode(VALUE val);
dpiPoolGetMode rbdpi_to_dpiPoolGetMode(VALUE val);
dpiPurity rbdpi_to_dpiPurity(VALUE val);
dpiShutdownMode rbdpi_to_dpiShutdownMode(VALUE val);
dpiStartupMode rbdpi_to_dpiStartupMode(VALUE val);
dpiSubscrNamespace rbdpi_to_dpiSubscrNamespace(VALUE val);
dpiSubscrProtocol rbdpi_to_dpiSubscrProtocol(VALUE val);
dpiSubscrQOS rbdpi_to_dpiSubscrQOS(VALUE val);
dpiVisibility rbdpi_to_dpiVisibility(VALUE val);

/* rbdpi-lob.c */
void Init_rbdpi_lob(VALUE mDpi);
VALUE rbdpi_from_lob(dpiLob *lob, const rbdpi_enc_t *enc, dpiOracleTypeNum oracle_type_num);
lob_t *rbdpi_to_lob(VALUE obj);

/* rbdpi-msg-props.c */
void Init_rbdpi_msg_props(VALUE mDpi);
VALUE rbdpi_from_msg_props(dpiMsgProps *msg_props, rb_encoding *enc);
msg_props_t *rbdpi_to_msg_props(VALUE obj);

/* rbdpi-object.c */
void Init_rbdpi_object(VALUE mDpi);
VALUE rbdpi_from_object(dpiObject *object, VALUE objtype);
object_t *rbdpi_to_object(VALUE obj);

/* rbdpi-object-attr.c */
void Init_rbdpi_object_attr(VALUE mDpi);
VALUE rbdpi_from_object_attr(dpiObjectAttr *attr, const rbdpi_enc_t *enc);
object_attr_t *rbdpi_to_object_attr(VALUE obj);

/* rbdpi-object-type.c */
void Init_rbdpi_object_type(VALUE mDpi);
VALUE rbdpi_from_object_type(dpiObjectType *type, const rbdpi_enc_t *enc);
object_type_t *rbdpi_to_object_type(VALUE obj);

/* rbdpi-pool.c */
void Init_rbdpi_pool(VALUE mDpi);
pool_t *rbdpi_to_pool(VALUE obj);

/* rbdpi-rowid.c */
void Init_rbdpi_rowid(VALUE mDpi);
VALUE rbdpi_from_rowid(dpiRowid *rowid);
rowid_t *rbdpi_to_rowid(VALUE obj);

/* rbdpi-stmt.c */
void Init_rbdpi_stmt(VALUE mDpi);
VALUE rbdpi_from_stmt(dpiStmt *stmt, const rbdpi_enc_t *enc);
stmt_t *rbdpi_to_stmt(VALUE obj);

/* rbdpi-struct.c */
void Init_rbdpi_struct(VALUE mDpi);
VALUE rbdpi_from_dpiEncodingInfo(const dpiEncodingInfo *info);
VALUE rbdpi_from_dpiErrorInfo(const dpiErrorInfo *error);
VALUE rbdpi_from_dpiIntervalDS(const dpiIntervalDS *intvl);
VALUE rbdpi_from_dpiIntervalYM(const dpiIntervalYM *intvl);
VALUE rbdpi_from_dpiQueryInfo(const dpiQueryInfo *info, const rbdpi_enc_t *enc);
VALUE rbdpi_from_dpiTimestamp(const dpiTimestamp *ts, dpiOracleTypeNum oracle_type);
void rbdpi_to_dpiIntervalDS(dpiIntervalDS *intvl, VALUE val);
void rbdpi_to_dpiIntervalYM(dpiIntervalYM *intvl, VALUE val);
void rbdpi_to_dpiTimestamp(dpiTimestamp *ts, VALUE val);

/* rbdpi-subscr.c */
void Init_rbdpi_subscr(VALUE mDpi);
VALUE rbdpi_subscr_prepare(subscr_t **out, dpiSubscrCreateParams *params, const rbdpi_enc_t *enc);
void rbdpi_subscr_start(subscr_t *subscr);

/* rbdpi-var.c */
void Init_rbdpi_var(VALUE mDpi);
var_t *rbdpi_to_var(VALUE obj);

/* rbdpi-version-info.c */
void Init_rbdpi_version_info(VALUE mDpi);
VALUE rbdpi_from_dpiVersionInfo(const dpiVersionInfo *ver);

#endif
