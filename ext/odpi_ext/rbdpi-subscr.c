#include "rbdpi.h"
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

static VALUE cSubscr;
static VALUE cSubscrMessage;
static VALUE cSubscrMessageQuery;
static VALUE cSubscrMessageTable;
static VALUE cSubscrMessageRow;

static ID id_at_db_name;
static ID id_at_error_info;
static ID id_at_event_type;
static ID id_at_id;
static ID id_at_name;
static ID id_at_operation;
static ID id_at_queries;
static ID id_at_rowid;
static ID id_at_rows;
static ID id_at_tables;

/* up to 8-byte boundary */
#define ROUND_UP(sz) ((sz + 7) & ~((size_t)7))

/* This must be allocated and freed by malloc and free, not by xmalloc and xfree */
typedef struct subscr_msg {
    struct subscr_msg *next;
    dpiSubscrMessage msg;
} subscr_msg_t;

struct subscr_callback_ctx {
    volatile long refcnt;
    volatile int closed;
    VALUE callback;
    rb_encoding *enc;
    mutex_t mutex;
#ifdef WIN32
    HANDLE hEvent;
#else
    int read_fd; /* non-blocking read-side pipe descriptor  */
    int write_fd; /* blocking write-side pipe descriptor */
#endif
    /* subscr_msg queue */
    subscr_msg_t *head;
    subscr_msg_t **tail;
};

typedef struct {
    int stage;
    dpiSubscrMessage *msg;
    dpiSubscrMessageQuery *query;
    dpiSubscrMessageTable *tbl;
    dpiSubscrMessageRow *row;
    dpiErrorInfo *err;
    char *chr;
} copy_ctx_t;

static subscr_callback_ctx_t *subscr_callback_ctx_alloc(VALUE proc, rb_encoding *enc);
static void subscr_callback_ctx_ref(subscr_callback_ctx_t *ctx);
static int subscr_callback_ctx_unref(subscr_callback_ctx_t *ctx);
static void subscr_callback_ctx_set_close(subscr_callback_ctx_t *ctx);
static void subscr_callback_ctx_notify(subscr_callback_ctx_t *ctx);
static void subscr_callback_ctx_wait(subscr_callback_ctx_t *ctx);

static subscr_msg_t *subscr_msg_alloc(const dpiSubscrMessage *msg);
static void subscr_msg_copy_msg(copy_ctx_t *ctx, const dpiSubscrMessage *msg);
static void subscr_msg_copy_msg_query(copy_ctx_t *ctx, const dpiSubscrMessageQuery *query);
static void subscr_msg_copy_msg_table(copy_ctx_t *ctx, const dpiSubscrMessageTable *tbl);
static void subscr_msg_copy_err(copy_ctx_t *ctx, const dpiErrorInfo *info);

static VALUE from_dpiSubscrMessage(const dpiSubscrMessage *msg, rb_encoding *enc);
static VALUE from_dpiSubscrMessageQuery(const dpiSubscrMessageQuery *query, rb_encoding *enc);
static VALUE from_dpiSubscrMessageTable(const dpiSubscrMessageTable *tbl, rb_encoding *enc);
static VALUE from_dpiSubscrMessageRow(const dpiSubscrMessageRow *row);

static subscr_callback_ctx_t *subscr_callback_ctx_alloc(VALUE proc, rb_encoding *enc)
{
    subscr_callback_ctx_t *ctx = xmalloc(sizeof(subscr_callback_ctx_t));
#ifndef WIN32
    int fds[2];
    int fd_flags;
#endif

    ctx->refcnt = 1;
    ctx->callback = proc;
    ctx->enc = enc;
    mutex_init(&ctx->mutex);
#ifdef WIN32
    ctx->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
    if (pipe(fds) != 0) {
        xfree(ctx);
        rb_sys_fail("pipe");
    }
    ctx->read_fd = fds[0];
    ctx->write_fd = fds[1];
    fd_flags = fcntl(ctx->read_fd, F_GETFL, 0);
    fcntl(ctx->read_fd, F_SETFL, fd_flags | O_NONBLOCK);
#endif
    ctx->head = NULL;
    ctx->tail = &ctx->head;
    return ctx;
}

static void subscr_callback_ctx_ref(subscr_callback_ctx_t *ctx)
{
    mutex_lock(&ctx->mutex);
    ctx->refcnt++;
    mutex_unlock(&ctx->mutex);
}

static int subscr_callback_ctx_unref(subscr_callback_ctx_t *ctx)
{
    long refcnt;

    mutex_lock(&ctx->mutex);
    refcnt = --ctx->refcnt;
    mutex_unlock(&ctx->mutex);
    if (refcnt == 0) {
        subscr_msg_t *msg, *msg_next;

        mutex_destroy(&ctx->mutex);
#ifdef WIN32
        CloseHandle(ctx->hEvent);
#else
        close(ctx->read_fd);
        close(ctx->write_fd);
#endif
        for (msg = ctx->head; msg != NULL; msg = msg_next) {
            msg_next = msg->next;
            free(msg);
        }
        xfree(ctx);
    }
    return refcnt;
}

static void subscr_callback_ctx_set_close(subscr_callback_ctx_t *ctx)
{
    mutex_lock(&ctx->mutex);
    ctx->closed = 1;
    mutex_unlock(&ctx->mutex);
    subscr_callback_ctx_notify(ctx);
}

static void subscr_callback_ctx_notify(subscr_callback_ctx_t *ctx)
{
#ifdef WIN32
        SetEvent(ctx->hEvent);
#else
        {
            char dummy = 0;
            int rv;

            do {
                rv = write(ctx->write_fd, &dummy, 1);
            } while (rv == -1 && errno == EINTR);
        }
#endif
}

static void subscr_callback_ctx_wait(subscr_callback_ctx_t *ctx)
{
#ifdef WIN32
    rb_w32_wait_events_blocking(&ctx->hEvent, 1, INFINITE);
    ResetEvent(ctx->hEvent);
#else
    char dummy[256];
    int rv;

    rb_thread_wait_fd(ctx->read_fd);
    do {
        rv = read(ctx->read_fd, dummy, sizeof(dummy));
    } while (rv > 0 || (rv == -1 && errno == EINTR));
#endif
}

static struct subscr_msg *subscr_msg_alloc(const dpiSubscrMessage *msg)
{
    copy_ctx_t ctx = {0,};
    subscr_msg_t *subscr_msg;
    char *buf;
    size_t sz;

    subscr_msg_copy_msg(&ctx, msg);

    sz = ROUND_UP(sizeof(subscr_msg_t))
        + ROUND_UP((size_t)ctx.query)
        + ROUND_UP((size_t)ctx.tbl)
        + ROUND_UP((size_t)ctx.row)
        + ROUND_UP((size_t)ctx.err)
        + ROUND_UP((size_t)ctx.chr);
    buf = malloc(sz);
    if (buf == NULL) {
        return NULL;
    }
    memset(buf, 0, sz);
    subscr_msg = (subscr_msg_t *)buf;
    subscr_msg->next = NULL;
    ctx.stage = 1;
    ctx.msg = &subscr_msg->msg;
    buf += ROUND_UP(sizeof(subscr_msg_t));
    sz = ROUND_UP((size_t)ctx.query);
    ctx.query = (dpiSubscrMessageQuery *)buf;
    buf += sz;
    sz = ROUND_UP((size_t)ctx.tbl);
    ctx.tbl = (dpiSubscrMessageTable *)buf;
    buf += sz;
    sz = ROUND_UP((size_t)ctx.row);
    ctx.row = (dpiSubscrMessageRow *)buf;
    buf += sz;
    if (ctx.err != NULL) {
        sz = ROUND_UP((size_t)ctx.err);
        ctx.err = (dpiErrorInfo *)buf;
        buf += sz;
    }
    ctx.chr = buf;

    subscr_msg_copy_msg(&ctx, msg);
    return subscr_msg;
}

static void subscr_msg_copy_msg(copy_ctx_t *ctx, const dpiSubscrMessage *msg)
{
    uint32_t idx;

    if (ctx->stage) {
        memcpy(ctx->msg, msg, sizeof(*msg));
        memcpy(ctx->chr, msg->dbName, msg->dbNameLength);
        ctx->msg->dbName = ctx->chr;
        ctx->msg->tables = ctx->tbl;
        ctx->msg->queries = ctx->query;
        ctx->msg->errorInfo = ctx->err;
    }
    ctx->chr += msg->dbNameLength;
    for (idx = 0; idx < msg->numTables; idx++) {
        subscr_msg_copy_msg_table(ctx, msg->tables + idx);
        ctx->tbl++;
    }
    for (idx = 0; idx < msg->numQueries; idx++) {
        subscr_msg_copy_msg_query(ctx, msg->queries + idx);
        ctx->query++;
    }
    if (msg->errorInfo) {
        subscr_msg_copy_err(ctx, msg->errorInfo);
        ctx->err++;
    }
}

static void subscr_msg_copy_msg_query(copy_ctx_t *ctx, const dpiSubscrMessageQuery *query)
{
    uint32_t idx;

    if (ctx->stage) {
        memcpy(ctx->query, query, sizeof(*query));
        ctx->query->tables = ctx->tbl;
    }
    for (idx = 0; idx < query->numTables; idx++) {
        subscr_msg_copy_msg_table(ctx, query->tables + idx);
        ctx->tbl++;
    }
}

static void subscr_msg_copy_msg_table(copy_ctx_t *ctx, const dpiSubscrMessageTable *tbl)
{
    if (ctx->stage) {
        memcpy(ctx->tbl, tbl, sizeof(*tbl));
        memcpy(ctx->chr, tbl->name, tbl->nameLength);
        memcpy(ctx->row, tbl->rows, tbl->numRows * sizeof(*tbl->rows));
        ctx->tbl->name = ctx->chr;
        ctx->tbl->rows = ctx->row;
    }
    ctx->chr += tbl->nameLength;
    ctx->row += tbl->numRows;
}

static void subscr_msg_copy_err(copy_ctx_t *ctx, const dpiErrorInfo *info)
{
    size_t encoding_len = info->encoding ? strlen(info->encoding) + 1 : 0;
    size_t fnName_len = info->fnName ? strlen(info->fnName) + 1 : 0;
    size_t action_len = info->action ? strlen(info->action) + 1 : 0;
    size_t sqlState_len = info->sqlState ? strlen(info->sqlState) + 1 : 0;

    if (ctx->stage) {
        memcpy(ctx->err, info, sizeof(*info));
        memcpy(ctx->chr, info->message, info->messageLength);
        ctx->err->message = ctx->chr;
    }
    ctx->chr += info->messageLength;
    if (ctx->stage) {
        memcpy(ctx->chr, info->encoding, encoding_len);
        ctx->err->encoding = ctx->chr;
    }
    ctx->chr += encoding_len;
    if (ctx->stage) {
        memcpy(ctx->chr, info->fnName, fnName_len);
        ctx->err->fnName = ctx->chr;
    }
    ctx->chr += fnName_len;
    if (ctx->stage) {
        memcpy(ctx->chr, info->action, action_len);
        ctx->err->action = ctx->chr;
    }
    ctx->chr += action_len;
    if (ctx->stage) {
        memcpy(ctx->chr, info->sqlState, sqlState_len);
        ctx->err->sqlState = ctx->chr;
    }
    ctx->chr += sqlState_len;
}

static VALUE from_dpiSubscrMessage(const dpiSubscrMessage *msg, rb_encoding *enc)
{
    VALUE obj = rb_obj_alloc(cSubscrMessage);
    VALUE ary;
    uint32_t idx;

    rb_ivar_set(obj, id_at_event_type, rbdpi_from_dpiEventType(msg->eventType));
    rb_ivar_set(obj, id_at_db_name, rb_external_str_new_with_enc(msg->dbName, msg->dbNameLength, enc));

    ary = rb_ary_new_capa(msg->numTables);
    for (idx = 0; idx < msg->numTables; idx++) {
        rb_ary_push(ary, from_dpiSubscrMessageTable(&msg->tables[idx], enc));
    }
    rb_ivar_set(obj, id_at_tables, ary);

    ary = rb_ary_new_capa(msg->numQueries);
    for (idx = 0; idx < msg->numQueries; idx++) {
        rb_ary_push(ary, from_dpiSubscrMessageQuery(&msg->queries[idx], enc));
    }
    rb_ivar_set(obj, id_at_queries, ary);

    rb_ivar_set(obj, id_at_error_info, msg->errorInfo ? rbdpi_from_dpiErrorInfo(msg->errorInfo) : Qnil);
    return obj;
}

static VALUE from_dpiSubscrMessageQuery(const dpiSubscrMessageQuery *query, rb_encoding *enc)
{
    VALUE obj = rb_obj_alloc(cSubscrMessageQuery);
    VALUE ary;
    uint32_t idx;

    rb_ivar_set(obj, id_at_id, ULL2NUM(query->id));
    rb_ivar_set(obj, id_at_operation, rbdpi_from_dpiOpCode(query->operation));

    ary = rb_ary_new_capa(query->numTables);
    for (idx = 0; idx < query->numTables; idx++) {
        rb_ary_push(ary, from_dpiSubscrMessageTable(&query->tables[idx], enc));
    }
    rb_ivar_set(obj, id_at_tables, ary);
    return obj;
}

static VALUE from_dpiSubscrMessageTable(const dpiSubscrMessageTable *tbl, rb_encoding *enc)
{
    VALUE obj = rb_obj_alloc(cSubscrMessageTable);
    VALUE ary;
    uint32_t idx;

    rb_ivar_set(obj, id_at_operation, rbdpi_from_dpiOpCode(tbl->operation));
    rb_ivar_set(obj, id_at_name, rb_external_str_new_with_enc(tbl->name, tbl->nameLength, enc));

    ary = rb_ary_new_capa(tbl->numRows);
    for (idx = 0; idx < tbl->numRows; idx++) {
        rb_ary_push(ary, from_dpiSubscrMessageRow(&tbl->rows[idx]));
    }
    rb_ivar_set(obj, id_at_rows, ary);
    return obj;
}

static VALUE from_dpiSubscrMessageRow(const dpiSubscrMessageRow *row)
{
    VALUE obj = rb_obj_alloc(cSubscrMessageRow);

    rb_ivar_set(obj, id_at_operation, rbdpi_from_dpiOpCode(row->operation));
    rb_ivar_set(obj, id_at_rowid, rb_usascii_str_new(row->rowid, row->rowidLength));
    return obj;
}

static void subscr_mark(void *arg)
{
    subscr_t *subscr = (subscr_t*)arg;

    if (subscr->callback_ctx != NULL) {
        rb_gc_mark(subscr->callback_ctx->callback);
    }
}

static void subscr_free(void *arg)
{
    subscr_t *subscr = (subscr_t*)arg;

    dpiSubscr_release(subscr->handle);
    if (subscr->callback_ctx != NULL) {
        if (subscr->callback_ctx->refcnt == 2) {
            subscr_callback_ctx_set_close(subscr->callback_ctx);
        }
        subscr_callback_ctx_unref(subscr->callback_ctx);
    }
    xfree(arg);
}

static const struct rb_data_type_struct subscr_data_type = {
    "ODPI::Dpi::Subscr",
    {subscr_mark, subscr_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static subscr_t *rbdpi_to_subscr(VALUE obj)
{
    subscr_t *subscr = (subscr_t *)rb_check_typeddata(obj, &subscr_data_type);

    if (subscr->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return subscr;
}

static VALUE subscr_alloc(VALUE klass)
{
    subscr_t *subscr;

    return TypedData_Make_Struct(klass, subscr_t, &subscr_data_type, subscr);
}

static VALUE subscr_initialize_copy(VALUE self, VALUE other)
{
    subscr_t *subscr = rbdpi_to_subscr(self);

    *subscr = *rbdpi_to_subscr(other);
    if (subscr->callback_ctx != NULL) {
        subscr_callback_ctx_ref(subscr->callback_ctx);
    }
    if (subscr->handle != NULL) {
        CHK(dpiSubscr_addRef(subscr->handle));
    }
    return self;
}

static VALUE subscr_id(VALUE self)
{
    return UINT2NUM(rbdpi_to_subscr(self)->subscr_id);
}

static VALUE subscr_close(VALUE self)
{
    subscr_t *subscr = rbdpi_to_subscr(self);

    CHK(dpiSubscr_close(subscr->handle));
    if (subscr->callback_ctx != NULL) {
        subscr_callback_ctx_set_close(subscr->callback_ctx);
        subscr_callback_ctx_unref(subscr->callback_ctx);
        subscr->callback_ctx = NULL;
    }
    return self;
}

static VALUE subscr_prepare_stmt(VALUE self, VALUE sql)
{
    subscr_t *subscr = rbdpi_to_subscr(self);
    dpiStmt *stmt;

    CHK_STR_ENC(sql, subscr->enc.enc);
    CHK(dpiSubscr_prepareStmt(subscr->handle, RSTRING_PTR(sql), RSTRING_LEN(sql), &stmt));
    RB_GC_GUARD(sql);
    return rbdpi_from_stmt(stmt, &subscr->enc);
}

void Init_rbdpi_subscr(VALUE mDpi)
{
    id_at_db_name = rb_intern("@db_name");
    id_at_error_info = rb_intern("@error_info");
    id_at_event_type = rb_intern("@event_type");
    id_at_id = rb_intern("@id");
    id_at_name = rb_intern("@name");
    id_at_operation = rb_intern("@operation");
    id_at_queries = rb_intern("@queries");
    id_at_rowid = rb_intern("@rowid");
    id_at_rows = rb_intern("@rows");
    id_at_tables = rb_intern("@tables");

    cSubscr = rb_define_class_under(mDpi, "Subscr", rb_cObject);
    rb_define_alloc_func(cSubscr, subscr_alloc);
    rb_define_method(cSubscr, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cSubscr, "initialize_copy", subscr_initialize_copy, 1);

    rb_define_method(cSubscr, "id", subscr_id, 0);
    rb_define_method(cSubscr, "close", subscr_close, 0);
    rb_define_method(cSubscr, "prepare_stmt", subscr_prepare_stmt, 1);

    cSubscrMessage = rb_define_class_under(cSubscr, "Message", rb_cObject);
    rb_define_attr(cSubscrMessage, "event_type", 1, 0);
    rb_define_attr(cSubscrMessage, "db_name", 1, 0);
    rb_define_attr(cSubscrMessage, "tables", 1, 0);
    rb_define_attr(cSubscrMessage, "queries", 1, 0);
    rb_define_attr(cSubscrMessage, "error_info", 1, 0);

    cSubscrMessageQuery = rb_define_class_under(cSubscrMessage, "Query", rb_cObject);
    rb_define_attr(cSubscrMessageQuery, "id", 1, 0);
    rb_define_attr(cSubscrMessageQuery, "operation", 1, 0);
    rb_define_attr(cSubscrMessageQuery, "tables", 1, 0);

    cSubscrMessageTable = rb_define_class_under(cSubscrMessage, "Table", rb_cObject);
    rb_define_attr(cSubscrMessageTable, "operation", 1, 0);
    rb_define_attr(cSubscrMessageTable, "name", 1, 0);
    rb_define_attr(cSubscrMessageTable, "rows", 1, 0);

    cSubscrMessageRow = rb_define_class_under(cSubscrMessage, "Row", rb_cObject);
    rb_define_attr(cSubscrMessageRow, "operation", 1, 0);
    rb_define_attr(cSubscrMessageRow, "rowid", 1, 0);
}

/* function called by the Oracle library.
 * Don't call ruby funtions in this callback.
 */
static void subscr_callback(void *context, dpiSubscrMessage *message)
{
    subscr_callback_ctx_t *ctx = (subscr_callback_ctx_t *)context;
    subscr_msg_t *msg = subscr_msg_alloc(message);

    if (msg != NULL) {
        mutex_lock(&ctx->mutex);
        *ctx->tail = msg;
        ctx->tail = &msg->next;
        mutex_unlock(&ctx->mutex);
        subscr_callback_ctx_notify(ctx);
    }
}

typedef struct {
    subscr_callback_ctx_t *ctx;
    subscr_msg_t *msg;
} cb_arg_t;

static VALUE run_callback_proc(VALUE data)
{
    cb_arg_t *arg = (cb_arg_t*)data;
    VALUE callback = arg->ctx->callback;
    VALUE msg = from_dpiSubscrMessage(&arg->msg->msg, arg->ctx->enc);

    return rb_proc_call(callback, rb_ary_new_from_args(1, msg));
}

static VALUE subscr_ruby_thread(subscr_callback_ctx_t *ctx)
{
    RB_UNUSED_VAR(volatile VALUE gc_guard) = ctx->callback;
    while (1) {
        subscr_callback_ctx_wait(ctx);

        if (ctx->closed) {
            break;
        }

        mutex_lock(&ctx->mutex);
        while (ctx->head != NULL) {
            subscr_msg_t *msg = ctx->head;
            cb_arg_t cb_arg;
            int state = 0;

            ctx->head = msg->next;
            if (ctx->tail == &msg->next) {
                ctx->tail = &ctx->head;
            }
            mutex_unlock(&ctx->mutex);
            cb_arg.ctx = ctx;
            cb_arg.msg = msg;
            rb_protect(run_callback_proc, (VALUE)&cb_arg, &state);
            if (state) {
                /* ignore errors */
                rb_set_errinfo(Qnil);
            }
            free(msg);
            mutex_lock(&ctx->mutex);
        }
        mutex_unlock(&ctx->mutex);
    }
    subscr_callback_ctx_unref(ctx);
    return Qnil;
}

VALUE rbdpi_subscr_prepare(subscr_t **out, dpiSubscrCreateParams *params, const rbdpi_enc_t *enc)
{
    subscr_t *subscr;
    VALUE obj = TypedData_Make_Struct(cSubscr, subscr_t, &subscr_data_type, subscr);

    subscr->enc = *enc;
    if (params->callback != NULL) {
        subscr->callback_ctx = subscr_callback_ctx_alloc((VALUE)params->callback, enc->enc);
        params->callback = subscr_callback;
        params->callbackContext = subscr->callback_ctx;
    }
    *out = subscr;
    return obj;
}

void rbdpi_subscr_start(subscr_t *subscr)
{
    if (subscr->callback_ctx != NULL) {
        subscr_callback_ctx_ref(subscr->callback_ctx);
        rb_thread_create(subscr_ruby_thread, subscr->callback_ctx);
    }
}
