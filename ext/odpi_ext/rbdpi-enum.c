#include "rbdpi.h"

typedef struct {
    VALUE sym;
    const char *name;
    int num;
} sym_cache_t;

static VALUE sym_unknown;

static void init_sym_cache(sym_cache_t *cache)
{
    while (cache->name != NULL) {
        cache->sym = ID2SYM(rb_intern(cache->name));
        cache++;
    }
}

static int sym_to_enum(VALUE val, int default_val, sym_cache_t *cache, int bit_flag, const char *type_name)
{
    if (default_val != -1 && NIL_P(val)) {
        return default_val;
    } else if (bit_flag && rb_type_p(val, T_ARRAY)) {
        long idx, len = RARRAY_LEN(val);
        int rv = 0;

        for (idx = 0; idx < len; idx++) {
            rv |= sym_to_enum(RARRAY_AREF(val, idx), default_val, cache, 0, type_name);
        }
        return rv;
    } else if (rb_type_p(val, T_SYMBOL)) {
        if (cache->sym == 0) {
            init_sym_cache(cache);
        }
        while (cache->name != NULL) {
            if (cache->sym == val) {
                return cache->num;
            }
            cache++;
        }
        rb_raise(rb_eArgError, "unknown %s: %s", type_name, rb_id2name(SYM2ID(val)));
    } else {
        rb_raise(rb_eTypeError, "wrong type %s (expect Symbol)",
                 rb_obj_classname(val));
    }
}

static VALUE enum_to_sym(int val, sym_cache_t *cache, int bit_flag, VALUE not_found_obj)
{
    if (cache->sym == 0) {
        init_sym_cache(cache);
    }
    if (bit_flag) {
        if (val == 0) {
            return Qnil;
        } else {
            VALUE ary = rb_ary_new();
            while (cache->name != NULL) {
                if (cache->num & val) {
                    rb_ary_push(ary, cache->sym);
                }
                cache++;
            }
            return ary;
        }
    } else {
        while (cache->name != NULL) {
            if (cache->num == val) {
                return cache->sym;
            }
            cache++;
        }
        return not_found_obj;
    }
}

void Init_rbdpi_enum(void)
{
    sym_unknown = ID2SYM(rb_intern("unknown"));
}

// connection/pool authorization modes
static sym_cache_t dpiAuthMode_cache[] = {
    {0, "default", DPI_MODE_AUTH_DEFAULT},
    {0, "sysdba", DPI_MODE_AUTH_SYSDBA},
    {0, "sysoper", DPI_MODE_AUTH_SYSOPER},
    {0, "prelim", DPI_MODE_AUTH_PRELIM},
    {0, "sysasm", DPI_MODE_AUTH_SYSASM},
    {0, NULL, 0},
};

// connection close modes
static sym_cache_t dpiConnCloseMode_cache[] = {
    {0, "default", DPI_MODE_CONN_CLOSE_DEFAULT},
    {0, "drop", DPI_MODE_CONN_CLOSE_DROP},
    {0, "retag", DPI_MODE_CONN_CLOSE_RETAG},
    {0, NULL, 0},
};

// connection/pool creation modes

// dequeue modes for advanced queuing
static sym_cache_t dpiDeqMode_cache[] = {
    {0, "browse", DPI_MODE_DEQ_BROWSE},
    {0, "locked", DPI_MODE_DEQ_LOCKED},
    {0, "remove", DPI_MODE_DEQ_REMOVE},
    {0, "remove_no_data", DPI_MODE_DEQ_REMOVE_NO_DATA},
    {0, NULL, 0},
};

// dequeue navigation flags for advanced queuing
static sym_cache_t dpiDeqNavigation_cache[] = {
    {0, "first_msg", DPI_DEQ_NAV_FIRST_MSG},
    {0, "next_transaction", DPI_DEQ_NAV_NEXT_TRANSACTION},
    {0, "next_msg", DPI_DEQ_NAV_NEXT_MSG},
    {0, NULL, 0},
};

// event types
static sym_cache_t dpiEventType_cache[] = {
    {0, "none", DPI_EVENT_NONE},
    {0, "startup", DPI_EVENT_STARTUP},
    {0, "shutdown", DPI_EVENT_SHUTDOWN},
    {0, "SHUTDOWN_any", DPI_EVENT_SHUTDOWN_ANY},
    {0, "DROP_db", DPI_EVENT_DROP_DB},
    {0, "dereg", DPI_EVENT_DEREG},
    {0, "objchange", DPI_EVENT_OBJCHANGE},
    {0, "querychange", DPI_EVENT_QUERYCHANGE},
    {0, NULL, 0},
};

// statement execution modes
static sym_cache_t dpiExecMode_cache[] = {
    {0, "default", DPI_MODE_EXEC_DEFAULT},
    {0, "describe_only", DPI_MODE_EXEC_DESCRIBE_ONLY},
    {0, "commit_on_success", DPI_MODE_EXEC_COMMIT_ON_SUCCESS},
    {0, "batch_errors", DPI_MODE_EXEC_BATCH_ERRORS},
    {0, "parse_only", DPI_MODE_EXEC_PARSE_ONLY},
    {0, "array_dml_rowcounts", DPI_MODE_EXEC_ARRAY_DML_ROWCOUNTS},
    {0, NULL, 0},
};

// statement fetch modes
static sym_cache_t dpiFetchMode_cache[] = {
    {0, "next", DPI_MODE_FETCH_NEXT},
    {0, "first", DPI_MODE_FETCH_FIRST},
    {0, "last", DPI_MODE_FETCH_LAST},
    {0, "prior", DPI_MODE_FETCH_PRIOR},
    {0, "absolute", DPI_MODE_FETCH_ABSOLUTE},
    {0, "relative", DPI_MODE_FETCH_RELATIVE},
    {0, NULL, 0},
};

// message delivery modes in advanced queuing
static sym_cache_t dpiMessageDeliveryMode_cache[] = {
    {0, "persistent", DPI_MODE_MSG_PERSISTENT},
    {0, "buffered", DPI_MODE_MSG_BUFFERED},
    {0, "persistent_or_buffered", DPI_MODE_MSG_PERSISTENT_OR_BUFFERED},
    {0, NULL, 0},
};

// message states in advanced queuing
static sym_cache_t dpiMessageState_cache[] = {
    {0, "ready", DPI_MSG_STATE_READY},
    {0, "waiting", DPI_MSG_STATE_WAITING},
    {0, "processed", DPI_MSG_STATE_PROCESSED},
    {0, "expired", DPI_MSG_STATE_EXPIRED},
    {0, NULL, 0},
};

// native C types
static sym_cache_t dpiNativeTypeNum_cache[] = {
    {0, "int64", DPI_NATIVE_TYPE_INT64},
    {0, "uint64", DPI_NATIVE_TYPE_UINT64},
    {0, "float", DPI_NATIVE_TYPE_FLOAT},
    {0, "double", DPI_NATIVE_TYPE_DOUBLE},
    {0, "bytes", DPI_NATIVE_TYPE_BYTES},
    {0, "timestamp", DPI_NATIVE_TYPE_TIMESTAMP},
    {0, "INTERVAL_ds", DPI_NATIVE_TYPE_INTERVAL_DS},
    {0, "INTERVAL_ym", DPI_NATIVE_TYPE_INTERVAL_YM},
    {0, "lob", DPI_NATIVE_TYPE_LOB},
    {0, "object", DPI_NATIVE_TYPE_OBJECT},
    {0, "stmt", DPI_NATIVE_TYPE_STMT},
    {0, "boolean", DPI_NATIVE_TYPE_BOOLEAN},
    {0, "rowid", DPI_NATIVE_TYPE_ROWID},
    {0, NULL, 0},
};

// operation codes (database change and continuous query notification)
static sym_cache_t dpiOpCode_cache[] = {
    {0, "all_ops", DPI_OPCODE_ALL_OPS},
    {0, "all_rows", DPI_OPCODE_ALL_ROWS},
    {0, "insert", DPI_OPCODE_INSERT},
    {0, "update", DPI_OPCODE_UPDATE},
    {0, "delete", DPI_OPCODE_DELETE},
    {0, "alter", DPI_OPCODE_ALTER},
    {0, "drop", DPI_OPCODE_DROP},
    {0, "unknown", DPI_OPCODE_UNKNOWN},
    {0, NULL, 0},
};

// Oracle types
static sym_cache_t dpiOracleTypeNum_cache[] = {
    {0, "none", DPI_ORACLE_TYPE_NONE},
    {0, "varchar", DPI_ORACLE_TYPE_VARCHAR},
    {0, "nvarchar", DPI_ORACLE_TYPE_NVARCHAR},
    {0, "char", DPI_ORACLE_TYPE_CHAR},
    {0, "nchar", DPI_ORACLE_TYPE_NCHAR},
    {0, "rowid", DPI_ORACLE_TYPE_ROWID},
    {0, "raw", DPI_ORACLE_TYPE_RAW},
    {0, "native_float", DPI_ORACLE_TYPE_NATIVE_FLOAT},
    {0, "native_double", DPI_ORACLE_TYPE_NATIVE_DOUBLE},
    {0, "native_int", DPI_ORACLE_TYPE_NATIVE_INT},
    {0, "number", DPI_ORACLE_TYPE_NUMBER},
    {0, "date", DPI_ORACLE_TYPE_DATE},
    {0, "timestamp", DPI_ORACLE_TYPE_TIMESTAMP},
    {0, "timestamp_tz", DPI_ORACLE_TYPE_TIMESTAMP_TZ},
    {0, "timestamp_ltz", DPI_ORACLE_TYPE_TIMESTAMP_LTZ},
    {0, "interval_ds", DPI_ORACLE_TYPE_INTERVAL_DS},
    {0, "interval_ym", DPI_ORACLE_TYPE_INTERVAL_YM},
    {0, "clob", DPI_ORACLE_TYPE_CLOB},
    {0, "nclob", DPI_ORACLE_TYPE_NCLOB},
    {0, "blob", DPI_ORACLE_TYPE_BLOB},
    {0, "bfile", DPI_ORACLE_TYPE_BFILE},
    {0, "stmt", DPI_ORACLE_TYPE_STMT},
    {0, "boolean", DPI_ORACLE_TYPE_BOOLEAN},
    {0, "object", DPI_ORACLE_TYPE_OBJECT},
    {0, "long_varchar", DPI_ORACLE_TYPE_LONG_VARCHAR},
    {0, "long_raw", DPI_ORACLE_TYPE_LONG_RAW},
    {0, "native_uint", DPI_ORACLE_TYPE_NATIVE_UINT},
    {0, NULL, 0},
};

// session pool close modes
static sym_cache_t dpiPoolCloseMode_cache[] = {
    {0, "default", DPI_MODE_POOL_CLOSE_DEFAULT},
    {0, "force", DPI_MODE_POOL_CLOSE_FORCE},
    {0, NULL, 0},
};

// modes used when acquiring a connection from a session pool
static sym_cache_t dpiPoolGetMode_cache[] = {
    {0, "wait", DPI_MODE_POOL_GET_WAIT},
    {0, "nowait", DPI_MODE_POOL_GET_NOWAIT},
    {0, "forceget", DPI_MODE_POOL_GET_FORCEGET},
    {0, NULL, 0},
};

// purity values when acquiring a connection from a pool
static sym_cache_t dpiPurity_cache[] = {
    {0, "default", DPI_PURITY_DEFAULT},
    {0, "new", DPI_PURITY_NEW},
    {0, "self", DPI_PURITY_SELF},
    {0, NULL, 0},
};

// database shutdown modes
static sym_cache_t dpiShutdownMode_cache[] = {
    {0, "default", DPI_MODE_SHUTDOWN_DEFAULT},
    {0, "transactional", DPI_MODE_SHUTDOWN_TRANSACTIONAL},
    {0, "transactional_local", DPI_MODE_SHUTDOWN_TRANSACTIONAL_LOCAL},
    {0, "immediate", DPI_MODE_SHUTDOWN_IMMEDIATE},
    {0, "abort", DPI_MODE_SHUTDOWN_ABORT},
    {0, "final", DPI_MODE_SHUTDOWN_FINAL},
    {0, NULL, 0},
};

// database startup modes
static sym_cache_t dpiStartupMode_cache[] = {
    {0, "default", DPI_MODE_STARTUP_DEFAULT},
    {0, "force", DPI_MODE_STARTUP_FORCE},
    {0, "restrict", DPI_MODE_STARTUP_RESTRICT},
    {0, NULL, 0},
};

// statement types
static sym_cache_t dpiStatementType_cache[] = {
    {0, "select", DPI_STMT_TYPE_SELECT},
    {0, "update", DPI_STMT_TYPE_UPDATE},
    {0, "delete", DPI_STMT_TYPE_DELETE},
    {0, "insert", DPI_STMT_TYPE_INSERT},
    {0, "create", DPI_STMT_TYPE_CREATE},
    {0, "drop", DPI_STMT_TYPE_DROP},
    {0, "alter", DPI_STMT_TYPE_ALTER},
    {0, "begin", DPI_STMT_TYPE_BEGIN},
    {0, "declare", DPI_STMT_TYPE_DECLARE},
    {0, "call", DPI_STMT_TYPE_CALL},
    {0, NULL, 0},
};

// subscription namespaces
static sym_cache_t dpiSubscrNamespace_cache[] = {
    {0, "dbchange", DPI_SUBSCR_NAMESPACE_DBCHANGE},
    {0, NULL, 0},
};

// subscription protocols
static sym_cache_t dpiSubscrProtocol_cache[] = {
    {0, "callback", DPI_SUBSCR_PROTO_CALLBACK},
    {0, "mail", DPI_SUBSCR_PROTO_MAIL},
    {0, "plsql", DPI_SUBSCR_PROTO_PLSQL},
    {0, "http", DPI_SUBSCR_PROTO_HTTP},
    {0, NULL, 0},
};

// subscription quality of service
static sym_cache_t dpiSubscrQOS_cache[] = {
    {0, "reliable", DPI_SUBSCR_QOS_RELIABLE},
    {0, "dereg_nfy", DPI_SUBSCR_QOS_DEREG_NFY},
    {0, "rowids", DPI_SUBSCR_QOS_ROWIDS},
    {0, "query", DPI_SUBSCR_QOS_QUERY},
    {0, "best_effort", DPI_SUBSCR_QOS_BEST_EFFORT},
    {0, NULL, 0},
};

// visibility of messages in advanced queuing
static sym_cache_t dpiVisibility_cache[] = {
    {0, "immediate", DPI_VISIBILITY_IMMEDIATE},
    {0, "on_commit", DPI_VISIBILITY_ON_COMMIT},
    {0, NULL, 0},
};

VALUE rbdpi_from_dpiAuthMode(dpiAuthMode val)
{
    return enum_to_sym(val, dpiAuthMode_cache, 1, sym_unknown);
}

VALUE rbdpi_from_dpiDeqMode(dpiDeqMode val)
{
    return enum_to_sym(val, dpiDeqMode_cache, 0, sym_unknown);
}

VALUE rbdpi_from_dpiDeqNavigation(dpiDeqNavigation val)
{
    return enum_to_sym(val, dpiDeqNavigation_cache, 0, sym_unknown);
}

VALUE rbdpi_from_dpiEventType(dpiEventType val)
{
    return enum_to_sym(val, dpiEventType_cache, 0, sym_unknown);
}

VALUE rbdpi_from_dpiMessageDeliveryMode(dpiMessageDeliveryMode val)
{
    return enum_to_sym(val, dpiMessageDeliveryMode_cache, 0, sym_unknown);
}

VALUE rbdpi_from_dpiMessageState(dpiMessageState val)
{
    return enum_to_sym(val, dpiMessageState_cache, 0, sym_unknown);
}

VALUE rbdpi_from_dpiNativeTypeNum(dpiNativeTypeNum val)
{
    return enum_to_sym(val, dpiNativeTypeNum_cache, 0, Qnil);
}

VALUE rbdpi_from_dpiOpCode(dpiOpCode val)
{
    return enum_to_sym(val, dpiOpCode_cache, 1, sym_unknown);
}

VALUE rbdpi_from_dpiOracleTypeNum(dpiOracleTypeNum val)
{
    return enum_to_sym(val, dpiOracleTypeNum_cache, 0, Qnil);
}

VALUE rbdpi_from_dpiPoolGetMode(dpiPoolGetMode val)
{
    return enum_to_sym(val, dpiPoolGetMode_cache, 0, sym_unknown);
}

VALUE rbdpi_from_dpiPurity(dpiPurity val)
{
    return enum_to_sym(val, dpiPurity_cache, 0, sym_unknown);
}

VALUE rbdpi_from_dpiStatementType(dpiStatementType val)
{
    return enum_to_sym(val, dpiStatementType_cache, 0, sym_unknown);
}

VALUE rbdpi_from_dpiSubscrNamespace(dpiSubscrNamespace val)
{
    return enum_to_sym(val, dpiSubscrNamespace_cache, 0, sym_unknown);
}

VALUE rbdpi_from_dpiSubscrProtocol(dpiSubscrProtocol val)
{
    return enum_to_sym(val, dpiSubscrProtocol_cache, 0, sym_unknown);
}

VALUE rbdpi_from_dpiSubscrQOS(dpiSubscrQOS val)
{
    return enum_to_sym(val, dpiSubscrQOS_cache, 1, sym_unknown);
}


VALUE rbdpi_from_dpiVisibility(dpiVisibility val)
{
    return enum_to_sym(val, dpiVisibility_cache, 0, sym_unknown);
}

dpiAuthMode rbdpi_to_dpiAuthMode(VALUE val)
{
    return sym_to_enum(val, DPI_MODE_AUTH_DEFAULT, dpiAuthMode_cache, 1, "auth mode");
}

dpiConnCloseMode rbdpi_to_dpiConnCloseMode(VALUE val)
{
    return sym_to_enum(val, DPI_MODE_CONN_CLOSE_DEFAULT, dpiConnCloseMode_cache, 1, "conn close mode");
}

dpiDeqMode rbdpi_to_dpiDeqMode(VALUE val)
{
    return sym_to_enum(val, -1, dpiDeqMode_cache, 0, "deq mode");
}

dpiDeqNavigation rbdpi_to_dpiDeqNavigation(VALUE val)
{
    return sym_to_enum(val, -1, dpiDeqNavigation_cache, 0, "deq navigation");
}

dpiExecMode rbdpi_to_dpiExecMode(VALUE val)
{
    return sym_to_enum(val, DPI_MODE_EXEC_DEFAULT, dpiExecMode_cache, 0, "exec mode");
}

dpiFetchMode rbdpi_to_dpiFetchMode(VALUE val)
{
    return sym_to_enum(val, DPI_MODE_FETCH_NEXT, dpiFetchMode_cache, 0, "fetch mode");
}

dpiMessageDeliveryMode rbdpi_to_dpiMessageDeliveryMode(VALUE val)
{
    return sym_to_enum(val, -1, dpiMessageDeliveryMode_cache, 0, "message delivery mode");
}

dpiNativeTypeNum rbdpi_to_dpiNativeTypeNum(VALUE val)
{
    return sym_to_enum(val, -1, dpiNativeTypeNum_cache, 0, "native type num");
}

dpiOpCode rbdpi_to_dpiOpCode(VALUE val)
{
    return sym_to_enum(val, -1, dpiOpCode_cache, 1, "op code");
}

dpiOracleTypeNum rbdpi_to_dpiOracleTypeNum(VALUE val)
{
    return sym_to_enum(val, -1, dpiOracleTypeNum_cache, 0, "oracle type");
}

dpiPoolCloseMode rbdpi_to_dpiPoolCloseMode(VALUE val)
{
    return sym_to_enum(val, DPI_MODE_POOL_CLOSE_DEFAULT, dpiPoolCloseMode_cache, 0, "pool close mode");
}

dpiPoolGetMode rbdpi_to_dpiPoolGetMode(VALUE val)
{
    return sym_to_enum(val, -1, dpiPoolGetMode_cache, 0, "pool get mode");
}

dpiPurity rbdpi_to_dpiPurity(VALUE val)
{
    return sym_to_enum(val, DPI_PURITY_DEFAULT, dpiPurity_cache, 0, "purity");
}

dpiShutdownMode rbdpi_to_dpiShutdownMode(VALUE val)
{
    return sym_to_enum(val, DPI_MODE_SHUTDOWN_DEFAULT, dpiShutdownMode_cache, 0, "shutdown mode");
}

dpiStartupMode rbdpi_to_dpiStartupMode(VALUE val)
{
    return sym_to_enum(val, DPI_MODE_STARTUP_DEFAULT, dpiStartupMode_cache, 0, "startup mode");
}

dpiSubscrNamespace rbdpi_to_dpiSubscrNamespace(VALUE val)
{
    return sym_to_enum(val, -1, dpiSubscrNamespace_cache, 0, "subscription namespace");
}

dpiSubscrProtocol rbdpi_to_dpiSubscrProtocol(VALUE val)
{
    return sym_to_enum(val, -1, dpiSubscrProtocol_cache, 0, "subscription protocol");
}

dpiSubscrQOS rbdpi_to_dpiSubscrQOS(VALUE val)
{
    return sym_to_enum(val, -1, dpiSubscrQOS_cache, 1, "subscription quality of service");
}

dpiVisibility rbdpi_to_dpiVisibility(VALUE val)
{
    return sym_to_enum(val, -1, dpiVisibility_cache, 0, "visibility");
}
