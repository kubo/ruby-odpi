#include "rbdpi.h"

static VALUE cVar;

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

    idx = NUM2UINT(pos);
    CHK(dpiVar_getData(var->handle, &num, &data));
    if (idx >= num) {
        rb_raise(rb_eRuntimeError, "out of array index %u for %u", idx, num);
    }
    return rbdpi_from_dpiData(data + idx, var->native_type, &var->enc, var->oracle_type, var->objtype);
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
        rbdpi_to_dpiData(data + idx, val, var->native_type, &var->enc, var->oracle_type);
    }
    return self;
}

void Init_rbdpi_var(VALUE mDpi)
{
    cVar = rb_define_class_under(mDpi, "Var", rb_cObject);
    rb_define_alloc_func(cVar, var_alloc);
    rb_define_method(cVar, "initialize", rbdpi_initialize_error, -1);
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
