/*
 * rbdpi-var.c -- part of ruby-odpi
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

static VALUE cVar;
static ID id_convert_in;
static ID id_convert_out;

static void var_mark(void *arg)
{
    var_t *var = (var_t*)arg;
    rb_gc_mark(var->objtype);
}

static void var_free(void *arg)
{
    var_t *var = (var_t*)arg;
    dpiVar_release(var->handle);
    xfree(arg);
}

static const struct rb_data_type_struct var_data_type = {
    "ODPI::Dpi::Var",
    {var_mark, var_free, },
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE var_alloc(VALUE klass)
{
    var_t *var;
    return TypedData_Make_Struct(klass, var_t, &var_data_type, var);
}

static VALUE var_initialize(VALUE self, VALUE rb_conn, VALUE oracle_type, VALUE native_type,
                          VALUE max_array_size, VALUE size, VALUE size_is_bytes,
                          VALUE is_array, VALUE objtype)
{
    var_t *var = (var_t *)rb_check_typeddata(self, &var_data_type);
    conn_t *conn = rbdpi_to_conn(rb_conn);
    dpiOracleTypeNum oracle_type_num = rbdpi_to_dpiOracleTypeNum(oracle_type);
    dpiNativeTypeNum native_type_num = rbdpi_to_dpiNativeTypeNum(native_type);
    dpiVar *handle;
    dpiData *data;

    if (var->handle != NULL) {
        rb_raise(rb_eRuntimeError, "Try to initialize an initialized connection");
    }

    CHK(dpiConn_newVar(conn->handle, oracle_type_num, native_type_num,
                       NUM2UINT(max_array_size), NUM2UINT(size), RTEST(size_is_bytes),
                       RTEST(is_array), NIL_P(objtype) ? NULL : rbdpi_to_object_type(objtype)->handle,
                       &handle, &data));
    var->handle = handle;
    var->enc = conn->enc;
    var->oracle_type = oracle_type_num;
    var->native_type = native_type_num;
    var->objtype = objtype;
    return Qnil;
}

static VALUE var_initialize_copy(VALUE self, VALUE other)
{
    var_t *var = rbdpi_to_var(self);
    *var = *rbdpi_to_var(other);
    if (var->handle != NULL) {
        CHK(dpiVar_addRef(var->handle));
    }
    return self;
}

static VALUE cvar_copy_data(VALUE self, VALUE pos, VALUE src, VALUE srcpos)
{
    var_t *dvar = rbdpi_to_var(self);
    var_t *svar = rbdpi_to_var(self);

    CHK(dpiVar_copyData(dvar->handle, NUM2UINT(pos), svar->handle, NUM2UINT(srcpos)));
    return self;
}

static VALUE cvar_get_num_elements_in_array(VALUE self)
{
    var_t *var = rbdpi_to_var(self);
    uint32_t num;

    CHK(dpiVar_getNumElementsInArray(var->handle, &num));
    return UINT2NUM(num);
}

static VALUE cvar_set_num_elements_in_array(VALUE self, VALUE num)
{
    var_t *var = rbdpi_to_var(self);

    CHK(dpiVar_setNumElementsInArray(var->handle, NUM2UINT(num)));
    return num;
}

static VALUE cvar_aref(VALUE self, VALUE pos)
{
    var_t *var = rbdpi_to_var(self);
    uint32_t idx, num;
    dpiData *data;
    VALUE val;

    idx = NUM2UINT(pos);
    CHK(dpiVar_getData(var->handle, &num, &data));
    if (idx >= num) {
        rb_raise(rb_eRuntimeError, "out of array index %u for %u", idx, num);
    }
    if (data[idx].isNull) {
        return Qnil;
    }
    val = rbdpi_from_dpiData2(data + idx, var->native_type, &var->enc, var->oracle_type, var->objtype);
    val = rb_funcall(self, id_convert_out, 1, val);
    return val;
}

static VALUE cvar_aset(VALUE self, VALUE pos, VALUE val)
{
    var_t *var = rbdpi_to_var(self);
    uint32_t idx, num;
    dpiData *data;

    idx = NUM2UINT(pos);
    CHK(dpiVar_getData(var->handle, &num, &data));
    if (idx >= num) {
        rb_raise(rb_eRuntimeError, "out of array index %u for %u", idx, num);
    }
    if (NIL_P(val)) {
        data[idx].isNull = 1;
        return self;
    }
    val = rb_funcall(self, id_convert_in, 1, val);
    switch (var->native_type) {
    case DPI_NATIVE_TYPE_BYTES:
        switch (rbdpi_ora2enc_type(var->oracle_type)) {
        case ENC_TYPE_CHAR:
            CHK_STR_ENC(val, var->enc.enc);
            break;
        case ENC_TYPE_NCHAR:
            CHK_STR_ENC(val, var->enc.nenc);
            break;
        case ENC_TYPE_OTHER:
            SafeStringValue(val);
            break;
        }
        CHK(dpiVar_setFromBytes(var->handle, idx, RSTRING_PTR(val), RSTRING_LEN(val)));
        RB_GC_GUARD(val);
        break;
    case DPI_NATIVE_TYPE_LOB:
        CHK(dpiVar_setFromLob(var->handle, idx, rbdpi_to_lob(val)->handle));
        break;
    case DPI_NATIVE_TYPE_OBJECT:
        CHK(dpiVar_setFromObject(var->handle, idx, rbdpi_to_object(val)->handle));
        break;
    case DPI_NATIVE_TYPE_ROWID:
        CHK(dpiVar_setFromRowid(var->handle, idx, rbdpi_to_rowid(val)->handle));
        break;
    case DPI_NATIVE_TYPE_STMT:
        CHK(dpiVar_setFromStmt(var->handle, idx, rbdpi_to_stmt(val)->handle));
        break;
    default:
        rbdpi_to_dpiData2(data + idx, val, var->native_type, &var->enc, var->oracle_type, var->objtype);
    }
    return self;
}

void Init_rbdpi_var(VALUE mDpi)
{
    id_convert_in = rb_intern("convert_in");
    id_convert_out = rb_intern("convert_out");

    cVar = rb_define_class_under(mDpi, "Var", rb_cObject);
    rb_define_alloc_func(cVar, var_alloc);
    rb_define_method(cVar, "initialize", var_initialize, 8);
    rb_define_method(cVar, "initialize_copy", var_initialize_copy, 1);
    rb_define_method(cVar, "copy_data", cvar_copy_data, 3);
    rb_define_method(cVar, "num_elements_in_array", cvar_get_num_elements_in_array, 0);
    rb_define_method(cVar, "num_elements_in_array=", cvar_set_num_elements_in_array, 1);
    rb_define_method(cVar, "[]", cvar_aref, 1);
    rb_define_method(cVar, "[]=", cvar_aset, 2);
}

VALUE rbdpi_from_var(dpiVar *handle, const rbdpi_enc_t *enc, dpiOracleTypeNum oracle_type, dpiNativeTypeNum native_type, VALUE objtype)
{
    var_t *var;
    VALUE obj = TypedData_Make_Struct(cVar, var_t, &var_data_type, var);

    var->handle = handle;
    var->enc = *enc;
    var->oracle_type = oracle_type;
    var->native_type = native_type;
    var->objtype = objtype;
    return obj;
}

var_t *rbdpi_to_var(VALUE obj)
{
    var_t *var = (var_t *)rb_check_typeddata(obj, &var_data_type);
    if (var->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return var;
}
