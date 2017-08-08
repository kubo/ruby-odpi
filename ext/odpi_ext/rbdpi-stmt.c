/*
 * rbdpi-stmt.c -- part of ruby-odpi
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

static VALUE cStmt;

static void stmt_mark(void *arg)
{
    stmt_t *stmt = (stmt_t *)arg;

    rb_gc_mark(stmt->query_columns_cache);
}

static void stmt_free(void *arg)
{
    stmt_t *stmt = (stmt_t *)arg;

    if (stmt->handle != NULL) {
        dpiStmt_release(stmt->handle);
    }
    xfree(arg);
}

static const struct rb_data_type_struct stmt_data_type = {
    "ODPI::Dpi::Stmt",
    {stmt_mark, stmt_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE stmt_alloc(VALUE klass)
{
    stmt_t *stmt;
    VALUE obj = TypedData_Make_Struct(cStmt, stmt_t, &stmt_data_type, stmt);

    stmt->query_columns_cache = Qfalse;
    return obj;
}

static VALUE stmt_initialize_copy(VALUE self, VALUE other)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    *stmt = *rbdpi_to_stmt(other);
    if (stmt->handle != NULL) {
        CHK(dpiStmt_addRef(stmt->handle));
    }
    return self;
}

static VALUE stmt_is_query_p(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    return stmt->info.isQuery ? Qtrue : Qfalse;
}

static VALUE stmt_is_plsql_p(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    return stmt->info.isPLSQL ? Qtrue : Qfalse;
}

static VALUE stmt_is_ddl_p(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    return stmt->info.isDDL ? Qtrue : Qfalse;
}

static VALUE stmt_is_dml_p(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    return rbdpi_from_dpiStatementType(stmt->info.statementType);
}

static VALUE stmt_get_statement_type(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    return stmt->info.isDML ? Qtrue : Qfalse;
}

static VALUE stmt_is_returning_p(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    return stmt->info.isReturning ? Qtrue : Qfalse;
}

static VALUE stmt_bind_by_name(VALUE self, VALUE name, VALUE var)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    CHK_STR_ENC(name, stmt->enc.enc);
    CHK(dpiStmt_bindByName(stmt->handle, RSTRING_PTR(name),
                           RSTRING_LEN(name), rbdpi_to_var(var)->handle));
    RB_GC_GUARD(name);
    return self;
}

static VALUE stmt_bind_by_pos(VALUE self, VALUE pos, VALUE var)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    CHK(dpiStmt_bindByPos(stmt->handle, NUM2UINT(pos), rbdpi_to_var(var)->handle));
    return self;
}

static VALUE stmt_close(VALUE self, VALUE tag)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    const char *tagptr = NULL;
    uint32_t taglen = 0;

    if (!NIL_P(tag)) {
        CHK_STR_ENC(tag, stmt->enc.enc);
        tagptr = RSTRING_PTR(tag);
        taglen = RSTRING_LEN(tag);
    }
    CHK(dpiStmt_close(stmt->handle, tagptr, taglen));
    RB_GC_GUARD(tag);
    return self;
}

static VALUE stmt_define(VALUE self, VALUE pos, VALUE var)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    CHK(dpiStmt_define(stmt->handle, NUM2UINT(pos), rbdpi_to_var(var)->handle));
    return self;
}

static VALUE stmt_execute(VALUE self, VALUE mode)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    uint32_t num_cols;

    CHK(dpiStmt_execute(stmt->handle, rbdpi_to_dpiExecMode(mode), &num_cols));
    return UINT2NUM(num_cols);
}

static VALUE stmt_execute_many(VALUE self, VALUE mode, VALUE num_iters)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    CHK(dpiStmt_executeMany(stmt->handle, rbdpi_to_dpiExecMode(mode), NUM2UINT(num_iters)));
    return Qnil;
}

static VALUE stmt_fetch(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    int found;
    uint32_t index;

    CHK(dpiStmt_fetch(stmt->handle, &found, &index));
    return found ? UINT2NUM(index) : Qnil;
}

static VALUE stmt_fetch_rows(VALUE self, VALUE max_rows)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    uint32_t index;
    uint32_t rows;
    int more_rows;

    CHK(dpiStmt_fetchRows(stmt->handle, NUM2UINT(max_rows), &index, &rows, &more_rows));
    if (rows) {
        return rb_ary_new_from_args(3, UINT2NUM(index), UINT2NUM(rows), more_rows ? Qtrue : Qfalse);
    } else {
        return Qnil;
    }
}

static VALUE stmt_get_batch_errors(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    uint32_t idx, count;
    dpiErrorInfo *errors;
    VALUE ary;

    CHK(dpiStmt_getBatchErrorCount(stmt->handle, &count));
    if (count == 0) {
        return rb_ary_new();
    }
    errors = ALLOCA_N(dpiErrorInfo, count);
    CHK(dpiStmt_getBatchErrors(stmt->handle, count, errors));
    ary = rb_ary_new_capa(count);
    for (idx = 0; idx < count; idx++) {
        rb_ary_push(ary, rbdpi_from_dpiErrorInfo(&errors[idx]));
    }
    return ary;
}

static VALUE stmt_get_bind_names(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    uint32_t idx, count;
    const char **names;
    uint32_t *lengths;
    VALUE ary;

    CHK(dpiStmt_getBindCount(stmt->handle, &count));
    names = ALLOCA_N(const char*, count);
    lengths = ALLOCA_N(uint32_t, count);
    CHK(dpiStmt_getBindNames(stmt->handle, &count, names, lengths));
    ary = rb_ary_new_capa(count);
    for (idx = 0; idx < count; idx++) {
        rb_ary_push(ary, rb_external_str_new_with_enc(names[idx], lengths[idx], stmt->enc.enc));
    }
    return self;
}

static VALUE stmt_get_fetch_array_size(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    uint32_t size;

    CHK(dpiStmt_getFetchArraySize(stmt->handle, &size));
    return UINT2NUM(size);
}

static VALUE stmt_get_implicit_result(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    dpiStmt *result;

    CHK(dpiStmt_getImplicitResult(stmt->handle, &result));
    if (result != NULL) {
        return rbdpi_from_stmt(result, &stmt->enc);
    } else {
        return Qnil;
    }
}

static VALUE stmt_get_query_columns(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    uint32_t num;

    if (stmt->query_columns_cache != Qfalse) {
        return stmt->query_columns_cache;
    }
    CHK(dpiStmt_getNumQueryColumns(stmt->handle, &num));
    if (num != 0) {
        VALUE ary = rb_ary_new_capa(num);
        uint32_t idx;

        for (idx = 1; idx <= num; idx++) {
            dpiQueryInfo info;

            CHK(dpiStmt_getQueryInfo(stmt->handle, idx, &info));
            rb_ary_push(ary, rbdpi_from_dpiQueryInfo(&info, &stmt->enc));
        }
        OBJ_FREEZE(ary);
        stmt->query_columns_cache = ary;
    } else {
        stmt->query_columns_cache = Qnil;
    }
    return stmt->query_columns_cache;
}

static VALUE stmt_get_row_count(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    uint64_t count;

    CHK(dpiStmt_getRowCount(stmt->handle, &count));
    return ULL2NUM(count);
}

static VALUE stmt_get_row_counts(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    uint32_t idx, numRowCounts;
    uint64_t *rowCounts;
    VALUE ary;

    CHK(dpiStmt_getRowCounts(stmt->handle, &numRowCounts, &rowCounts));
    ary = rb_ary_new_capa(numRowCounts);
    for (idx = 0; idx < numRowCounts; idx++) {
        rb_ary_store(ary, idx, ULL2NUM(rowCounts[idx]));
    }
    return ary;
}

static VALUE stmt_get_subscr_query_id(VALUE self)
{
    stmt_t *stmt = rbdpi_to_stmt(self);
    uint64_t query_id;

    CHK(dpiStmt_getSubscrQueryId(stmt->handle, &query_id));
    return ULL2NUM(query_id);
}

static VALUE stmt_scroll(VALUE self, VALUE mode, VALUE offset, VALUE row_count_offset)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    CHK(dpiStmt_scroll(stmt->handle, rbdpi_to_dpiFetchMode(mode), NUM2INT(offset), NUM2INT(row_count_offset)));
    return self;
}

static VALUE stmt_set_fetch_array_size(VALUE self, VALUE val)
{
    stmt_t *stmt = rbdpi_to_stmt(self);

    CHK(dpiStmt_setFetchArraySize(stmt->handle, NUM2UINT(val)));
    return val;
}

void Init_rbdpi_stmt(VALUE mDpi)
{
    cStmt = rb_define_class_under(mDpi, "Stmt", rb_cObject);
    rb_define_alloc_func(cStmt, stmt_alloc);
    rb_define_method(cStmt, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cStmt, "initialize_copy", stmt_initialize_copy, 1);
    rb_define_method(cStmt, "query?", stmt_is_query_p, 0);
    rb_define_method(cStmt, "plsql?", stmt_is_plsql_p, 0);
    rb_define_method(cStmt, "ddl?", stmt_is_ddl_p, 0);
    rb_define_method(cStmt, "dml?", stmt_is_dml_p, 0);
    rb_define_method(cStmt, "statement_type", stmt_get_statement_type, 0);
    rb_define_method(cStmt, "returning?", stmt_is_returning_p, 0);
    rb_define_method(cStmt, "bind_by_name", stmt_bind_by_name, 2);
    rb_define_method(cStmt, "bind_by_pos", stmt_bind_by_pos, 2);
    rb_define_method(cStmt, "close", stmt_close, 1);
    rb_define_method(cStmt, "define", stmt_define, 2);
    rb_define_method(cStmt, "execute", stmt_execute, 1);
    rb_define_method(cStmt, "execute_many", stmt_execute_many, 2);
    rb_define_method(cStmt, "fetch", stmt_fetch, 0);
    rb_define_method(cStmt, "fetch_rows", stmt_fetch_rows, 1);
    rb_define_method(cStmt, "batch_errors", stmt_get_batch_errors, 0);
    rb_define_method(cStmt, "bind_names", stmt_get_bind_names, 0);
    rb_define_method(cStmt, "fetch_array_size", stmt_get_fetch_array_size, 0);
    rb_define_method(cStmt, "implicit_result", stmt_get_implicit_result, 0);
    rb_define_method(cStmt, "query_columns", stmt_get_query_columns, 0);
    rb_define_method(cStmt, "row_count", stmt_get_row_count, 0);
    rb_define_method(cStmt, "row_counts", stmt_get_row_counts, 0);
    rb_define_method(cStmt, "subscr_query_id", stmt_get_subscr_query_id, 0);
    rb_define_method(cStmt, "scroll", stmt_scroll, 3);
    rb_define_method(cStmt, "fetch_array_size=", stmt_set_fetch_array_size, 1);
}

VALUE rbdpi_from_stmt(dpiStmt *handle, const rbdpi_enc_t *enc)
{
    stmt_t *stmt;
    VALUE obj = TypedData_Make_Struct(cStmt, stmt_t, &stmt_data_type, stmt);

    stmt->handle = handle;
    stmt->enc = *enc;
    CHK(dpiStmt_getInfo(handle, &stmt->info));
    return obj;
}

stmt_t *rbdpi_to_stmt(VALUE obj)
{
    stmt_t *stmt = (stmt_t *)rb_check_typeddata(obj, &stmt_data_type);

    if (stmt->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return stmt;
}
