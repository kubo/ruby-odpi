#include "rbdpi.h"

static VALUE cRowid;

static void rowid_free(void *arg)
{
    dpiRowid_release(((rowid_t*)arg)->handle);
    xfree(arg);
}

static const struct rb_data_type_struct rowid_data_type = {
    "ODPI::Dpi::Rowid",
    {NULL, rowid_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE rowid_alloc(VALUE klass)
{
    rowid_t *rowid;

    return TypedData_Make_Struct(klass, rowid_t, &rowid_data_type, rowid);
}

static VALUE rowid_initialize_copy(VALUE self, VALUE other)
{
    rowid_t *rowid = rbdpi_to_rowid(self);

    *rowid = *rbdpi_to_rowid(other);
    if (rowid->handle != NULL) {
        CHK(dpiRowid_addRef(rowid->handle));
    }
    return self;
}

static VALUE rowid_to_s(VALUE self)
{
    rowid_t *rowid = rbdpi_to_rowid(self);
    const char *val;
    uint32_t len;

    CHK(dpiRowid_getStringValue(rowid->handle, &val, &len));
    return rb_usascii_str_new(val, len);
}

void Init_rbdpi_rowid(VALUE mDpi)
{
    cRowid = rb_define_class_under(mDpi, "Rowid", rb_cObject);
    rb_define_alloc_func(cRowid, rowid_alloc);
    rb_define_method(cRowid, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cRowid, "initialize_copy", rowid_initialize_copy, 1);
    rb_define_method(cRowid, "to_s", rowid_to_s, 0);
}

VALUE rbdpi_from_rowid(dpiRowid *handle)
{
    rowid_t *rowid;
    VALUE obj = TypedData_Make_Struct(cRowid, rowid_t, &rowid_data_type, rowid);

    rowid->handle = handle;
    return obj;
}

rowid_t *rbdpi_to_rowid(VALUE obj)
{
    rowid_t *rowid = (rowid_t *)rb_check_typeddata(obj, &rowid_data_type);

    if (rowid->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return rowid;
}
