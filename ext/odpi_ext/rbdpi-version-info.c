#include "rbdpi.h"

static VALUE cVersionInfo;
static ID id_at_version;

static VALUE version_info_initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE vernum;

    if (argc == 1) {
        VALUE arg = argv[0];
        if (FIXNUM_P(arg)) {
            vernum = arg;
        } else {
            int version = 0, release = 0, update = 0, port_release = 0, port_update = 0;
            char *s = StringValueCStr(arg);
            sscanf(s, "%d.%d.%d.%d.%d", &version, &release, &update, &port_release, &port_update);
            vernum = INT2FIX(DPI_ORACLE_VERSION_TO_NUMBER(version, release, update, port_release, port_update));
        }
    } else {
        int ver;
        Check_Type(argv[0], T_FIXNUM);
        ver = FIX2INT(argv[0]) * 100000000;
        if (argc > 1) {
            Check_Type(argv[1], T_FIXNUM);
            ver += FIX2INT(argv[1]) * 1000000;
        }
        if (argc > 2) {
            Check_Type(argv[2], T_FIXNUM);
            ver += FIX2INT(argv[2]) * 10000;
        }
        if (argc > 3) {
            Check_Type(argv[3], T_FIXNUM);
            ver += FIX2INT(argv[3]) * 100;
        }
        if (argc > 4) {
            Check_Type(argv[4], T_FIXNUM);
            ver += FIX2INT(argv[4]);
        }
        vernum = INT2FIX(ver);
    }
    rb_ivar_set(self, id_at_version, vernum);
    return self;
}

static VALUE version_info_inspect(VALUE self)
{
    int vernum = FIX2INT(rb_ivar_get(self, id_at_version));

    return rb_sprintf("<%s: %d.%d.%d.%d.%d>",
                      rb_obj_classname(self),
                      (vernum / 100000000),
                      (vernum / 1000000) % 100,
                      (vernum / 10000) % 100,
                      (vernum / 100) % 100,
                      (vernum / 1) % 100);
}

static VALUE version_info_to_i(VALUE self)
{
    return rb_ivar_get(self, id_at_version);
}

static VALUE version_info_to_s(VALUE self)
{
    int vernum = FIX2INT(rb_ivar_get(self, id_at_version));

    return rb_sprintf("%d.%d.%d.%d.%d",
                      (vernum / 100000000),
                      (vernum / 1000000) % 100,
                      (vernum / 10000) % 100,
                      (vernum / 100) % 100,
                      (vernum / 1) % 100);
}

static VALUE version_info_cmp(VALUE self, VALUE other)
{
    int ver = FIX2INT(rb_ivar_get(self, id_at_version));
    int other_ver;

    if (!rb_obj_is_kind_of(cVersionInfo, other)) {
        return Qnil;
    }
    other_ver = FIX2INT(rb_ivar_get(other, id_at_version));
    if (ver == other_ver) {
        return INT2FIX(0);
    }
    if (ver > other_ver) {
        return INT2FIX(1);
    }
    return INT2FIX(-1);
}

void Init_rbdpi_version_info(VALUE mDpi)
{
    id_at_version = rb_intern("@version");

    cVersionInfo = rb_define_class_under(mDpi, "VersionInfo", rb_cObject);
    rb_define_method(cVersionInfo, "initialize", version_info_initialize, -1);
    rb_define_method(cVersionInfo, "inspect", version_info_inspect, 0);
    rb_define_method(cVersionInfo, "to_i", version_info_to_i, 0);
    rb_define_method(cVersionInfo, "to_s", version_info_to_s, 0);
    rb_define_method(cVersionInfo, "<=>", version_info_cmp, 1);
    rb_include_module(cVersionInfo, rb_mComparable);
}

VALUE rbdpi_from_dpiVersionInfo(const dpiVersionInfo *ver)
{
    VALUE obj = rb_obj_alloc(cVersionInfo);
    rb_ivar_set(obj, id_at_version, INT2FIX(ver->fullVersionNum));
    return obj;
}
