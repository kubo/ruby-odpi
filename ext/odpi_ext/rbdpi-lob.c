#include "rbdpi.h"

static VALUE cLob;

static void lob_free(void *arg)
{
    dpiLob_release(((lob_t*)arg)->handle);
    xfree(arg);
}

static const struct rb_data_type_struct lob_data_type = {
    "ODPI::Dpi::Lob",
    {NULL, lob_free,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE lob_alloc(VALUE klass)
{
    lob_t *lob;

    return TypedData_Make_Struct(klass, lob_t, &lob_data_type, lob);
}

static VALUE lob_initialize_copy(VALUE self, VALUE other)
{
    lob_t *lob = rbdpi_to_lob(self);

    *lob = *rbdpi_to_lob(other);
    if (lob->handle != NULL) {
        dpiLob *oldlob = lob->handle;

        lob->handle = NULL;
        CHK(dpiLob_copy(oldlob, &lob->handle));
    }
    return self;
}

static VALUE lob_close(VALUE self)
{
    lob_t *lob = rbdpi_to_lob(self);

    CHK(dpiLob_close(lob->handle));
    return self;
}

static VALUE lob_close_resource(VALUE self)
{
    lob_t *lob = rbdpi_to_lob(self);

    CHK(dpiLob_closeResource(lob->handle));
    return self;
}

static VALUE lob_flush_buffer(VALUE self)
{
    lob_t *lob = rbdpi_to_lob(self);

    CHK(dpiLob_flushBuffer(lob->handle));
    return self;
}

static VALUE lob_get_buffer_size(VALUE self, VALUE size_in_chars)
{
    lob_t *lob = rbdpi_to_lob(self);
    uint64_t size_in_bytes;

    CHK(dpiLob_getBufferSize(lob->handle, NUM2ULL(size_in_chars), &size_in_bytes));
    return ULL2NUM(size_in_bytes);
}

static VALUE lob_get_chunk_size(VALUE self)
{
    lob_t *lob = rbdpi_to_lob(self);
    uint32_t size;

    CHK(dpiLob_getChunkSize(lob->handle, &size));
    return UINT2NUM(size);
}

static VALUE lob_get_directory_and_file_name(VALUE self)
{
    lob_t *lob = rbdpi_to_lob(self);
    const char *dir;
    uint32_t dirlen;
    const char *fname;
    uint32_t fnamelen;

    CHK(dpiLob_getDirectoryAndFileName(lob->handle, &dir, &dirlen, &fname, &fnamelen));
    return rb_ary_new_from_args(2,
                                rb_external_str_new_with_enc(dir, dirlen, lob->enc.enc),
                                rb_external_str_new_with_enc(fname, fnamelen, lob->enc.enc));
}

static VALUE lob_file_exists_p(VALUE self)
{
    lob_t *lob = rbdpi_to_lob(self);
    int exists;

    CHK(dpiLob_getFileExists(lob->handle, &exists));
    return exists ? Qtrue : Qfalse;
}

static VALUE lob_is_resource_open_p(VALUE self)
{
    lob_t *lob = rbdpi_to_lob(self);
    int is_open;

    CHK(dpiLob_getIsResourceOpen(lob->handle, &is_open));
    return is_open ? Qtrue : Qfalse;
}

static VALUE lob_get_size(VALUE self)
{
    lob_t *lob = rbdpi_to_lob(self);
    uint64_t size;

    CHK(dpiLob_getSize(lob->handle, &size));
    return ULL2NUM(size);
}

static VALUE lob_open_resource(VALUE self)
{
    lob_t *lob = rbdpi_to_lob(self);

    CHK(dpiLob_openResource(lob->handle));
    return self;
}

static VALUE lob_read(VALUE self, VALUE offset, VALUE amount)
{
    lob_t *lob = rbdpi_to_lob(self);
    uint64_t off = NUM2ULL(offset);
    uint64_t amt = NUM2ULL(amount);
    VALUE str;
    uint64_t len;

    CHK(dpiLob_getBufferSize(lob->handle, amt, &len));
    str = rb_str_buf_new(len);

    CHK(dpiLob_readBytes(lob->handle, off, amt, RSTRING_PTR(str), &len));
    rb_str_set_len(str, len);
    switch (lob->oracle_type) {
    case DPI_ORACLE_TYPE_CLOB:
        rb_enc_associate(str, lob->enc.enc);
        break;
    case DPI_ORACLE_TYPE_NCLOB:
        rb_enc_associate(str, lob->enc.nenc);
        break;
    default:
        break;
    }
    OBJ_TAINT(str);
    return str;
}

static VALUE lob_set_directory_and_file_name(VALUE self, VALUE dir, VALUE fname)
{
    lob_t *lob = rbdpi_to_lob(self);

    CHK_STR_ENC(dir, lob->enc.enc);
    CHK_STR_ENC(fname, lob->enc.enc);

    CHK(dpiLob_setDirectoryAndFileName(lob->handle,
                                       RSTRING_PTR(dir), RSTRING_LEN(dir),
                                       RSTRING_PTR(fname), RSTRING_LEN(fname)));
    RB_GC_GUARD(dir);
    RB_GC_GUARD(fname);
    return self;
}

static VALUE lob_set(VALUE self, VALUE val)
{
    lob_t *lob = rbdpi_to_lob(self);

    switch (lob->oracle_type) {
    case DPI_ORACLE_TYPE_CLOB:
        CHK_STR_ENC(val, lob->enc.enc);
        break;
    case DPI_ORACLE_TYPE_NCLOB:
        CHK_STR_ENC(val, lob->enc.nenc);
        break;
    default:
        SafeStringValue(val);
    }

    CHK(dpiLob_setFromBytes(lob->handle, RSTRING_PTR(val), RSTRING_LEN(val)));
    RB_GC_GUARD(val);
    return self;
}

static VALUE lob_trim(VALUE self, VALUE new_size)
{
    lob_t *lob = rbdpi_to_lob(self);

    CHK(dpiLob_trim(lob->handle, NUM2ULL(new_size)));
    return self;
}

static VALUE lob_write(VALUE self, VALUE offset, VALUE val)
{
    lob_t *lob = rbdpi_to_lob(self);

    switch (lob->oracle_type) {
    case DPI_ORACLE_TYPE_CLOB:
        CHK_STR_ENC(val, lob->enc.enc);
        break;
    case DPI_ORACLE_TYPE_NCLOB:
        CHK_STR_ENC(val, lob->enc.nenc);
        break;
    default:
        SafeStringValue(val);
    }

    CHK(dpiLob_writeBytes(lob->handle, NUM2ULL(offset), RSTRING_PTR(val), RSTRING_LEN(val)));
    RB_GC_GUARD(val);
    return self;
}

void Init_rbdpi_lob(VALUE mDpi)
{
    cLob = rb_define_class_under(mDpi, "Lob", rb_cObject);
    rb_define_alloc_func(cLob, lob_alloc);
    rb_define_method(cLob, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cLob, "initialize_copy", lob_initialize_copy, 1);
    rb_define_method(cLob, "close", lob_close, 0);
    rb_define_method(cLob, "close_resource", lob_close_resource, 0);
    rb_define_method(cLob, "flush_buffer", lob_flush_buffer, 0);
    rb_define_method(cLob, "buffer_size", lob_get_buffer_size, 1);
    rb_define_method(cLob, "chunk_size", lob_get_chunk_size, 0);
    rb_define_method(cLob, "directory_and_file_name", lob_get_directory_and_file_name, 0);
    rb_define_method(cLob, "file_exists?", lob_file_exists_p, 0);
    rb_define_method(cLob, "is_resource_open?", lob_is_resource_open_p, 0);
    rb_define_method(cLob, "size", lob_get_size, 0);
    rb_define_method(cLob, "open_resource", lob_open_resource, 0);
    rb_define_method(cLob, "read", lob_read, 2);
    rb_define_method(cLob, "set_directory_and_file_name", lob_set_directory_and_file_name, 2);
    rb_define_method(cLob, "set", lob_set, 1);
    rb_define_method(cLob, "trim", lob_trim, 1);
    rb_define_method(cLob, "write", lob_write, 2);
}

VALUE rbdpi_from_lob(dpiLob *handle, const rbdpi_enc_t *enc, dpiOracleTypeNum oracle_type)
{
    lob_t *lob;
    VALUE obj = TypedData_Make_Struct(cLob, lob_t, &lob_data_type, lob);

    lob->handle = handle;
    lob->enc = *enc;
    lob->oracle_type = oracle_type;
    return obj;
}

lob_t *rbdpi_to_lob(VALUE obj)
{
    lob_t *lob = (lob_t *)rb_check_typeddata(obj, &lob_data_type);

    if (lob->handle == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return lob;
}
