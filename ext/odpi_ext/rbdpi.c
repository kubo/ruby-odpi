#include "rbdpi.h"

const dpiContext *rbdpi_g_context;

VALUE rbdpi_initialize_error(VALUE self)
{
    rb_raise(rb_eRuntimeError, "could not initialize by %s::new", rb_obj_classname(self));
    return Qnil;
}

/* ODPI::oracle_client_version */
static VALUE oracle_client_version(VALUE klass)
{
    dpiVersionInfo ver;

    CHK(dpiContext_getClientVersion(rbdpi_g_context, &ver));
    return rbdpi_from_dpiVersionInfo(&ver);
}

void
Init_odpi_ext()
{
    VALUE mODPI = rb_define_module("ODPI");
    VALUE mDpi = rb_define_module_under(mODPI, "Dpi");
    dpiErrorInfo error;
    dpiContext *context;

    if (dpiContext_create(DPI_MAJOR_VERSION, DPI_MINOR_VERSION, &context, &error) != DPI_SUCCESS) {
        rb_raise(rb_eLoadError, "%.*s", error.messageLength, error.message);
    }
    rbdpi_g_context = context;

    rb_define_const(mDpi, "ODPI_C_VERSION", rb_usascii_str_new_cstr(DPI_VERSION_STRING));
    rb_define_singleton_method(mDpi, "oracle_client_version", oracle_client_version, 0);

    Init_rbdpi_conn(mDpi);
    Init_rbdpi_deq_options(mDpi);
    Init_rbdpi_enq_options(mDpi);
    Init_rbdpi_enum();
    Init_rbdpi_lob(mDpi);
    Init_rbdpi_msg_props(mDpi);
    Init_rbdpi_object(mDpi);
    Init_rbdpi_object_attr(mDpi);
    Init_rbdpi_object_type(mDpi);
    Init_rbdpi_pool(mDpi);
    Init_rbdpi_rowid(mDpi);
    Init_rbdpi_stmt(mDpi);
    Init_rbdpi_struct(mDpi);
    Init_rbdpi_subscr(mDpi);
    Init_rbdpi_var(mDpi);
    Init_rbdpi_version_info(mDpi);
}
