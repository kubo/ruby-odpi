#include "rbdpi.h"

VALUE rbdpi_from_dpiData(const dpiData *data, dpiNativeTypeNum type, const rbdpi_enc_t *enc, dpiOracleTypeNum oratype, VALUE objtype)
{
    if (data->isNull) {
        return Qnil;
    }
    switch (type) {
    case DPI_NATIVE_TYPE_INT64:
        return LL2NUM(data->value.asInt64);
    case DPI_NATIVE_TYPE_UINT64:
        return ULL2NUM(data->value.asUint64);
    case DPI_NATIVE_TYPE_FLOAT:
        return DBL2NUM(data->value.asFloat);
    case DPI_NATIVE_TYPE_DOUBLE:
        return DBL2NUM(data->value.asDouble);
    case DPI_NATIVE_TYPE_BYTES:
        switch (rbdpi_ora2enc_type(oratype)) {
        case ENC_TYPE_CHAR:
            return rb_external_str_new_with_enc(data->value.asBytes.ptr, data->value.asBytes.length, enc->enc);
        case ENC_TYPE_NCHAR:
            return rb_external_str_new_with_enc(data->value.asBytes.ptr, data->value.asBytes.length, enc->nenc);
        case ENC_TYPE_OTHER:
            return rb_tainted_str_new(data->value.asBytes.ptr, data->value.asBytes.length);
        }
    case DPI_NATIVE_TYPE_TIMESTAMP:
        return rbdpi_from_dpiTimestamp(&data->value.asTimestamp);
    case DPI_NATIVE_TYPE_INTERVAL_DS:
        return rbdpi_from_dpiIntervalDS(&data->value.asIntervalDS);
    case DPI_NATIVE_TYPE_INTERVAL_YM:
        return rbdpi_from_dpiIntervalYM(&data->value.asIntervalYM);
    case DPI_NATIVE_TYPE_LOB:
        CHK(dpiLob_addRef(data->value.asLOB));
        return rbdpi_from_lob(data->value.asLOB, enc, oratype);
    case DPI_NATIVE_TYPE_OBJECT:
        CHK(dpiObject_addRef(data->value.asObject));
        return rbdpi_from_object(data->value.asObject, objtype);
    case DPI_NATIVE_TYPE_STMT:
        CHK(dpiStmt_addRef(data->value.asStmt));
        return rbdpi_from_stmt(data->value.asStmt, enc);
    case DPI_NATIVE_TYPE_BOOLEAN:
        return data->value.asBoolean ? Qtrue : Qfalse;
    case DPI_NATIVE_TYPE_ROWID:
        CHK(dpiRowid_addRef(data->value.asRowid));
        return rbdpi_from_rowid(data->value.asRowid);
    }
    rb_raise(rb_eRuntimeError, "unknown native type %d", type);
}

VALUE rbdpi_to_dpiData(dpiData *data, VALUE val, dpiNativeTypeNum type, const rbdpi_enc_t *enc, dpiOracleTypeNum oratype)
{
    if (NIL_P(val)) {
        data->isNull = 1;
    } else {
        data->isNull = 0;
        switch (type) {
        case DPI_NATIVE_TYPE_INT64:
            data->value.asInt64 = NUM2LL(val);
            break;
        case DPI_NATIVE_TYPE_UINT64:
            data->value.asUint64 = NUM2ULL(val);
            break;
        case DPI_NATIVE_TYPE_FLOAT:
            data->value.asFloat = NUM2DBL(val);
            break;
        case DPI_NATIVE_TYPE_DOUBLE:
            data->value.asDouble = NUM2DBL(val);
            break;
        case DPI_NATIVE_TYPE_BYTES:
            switch (rbdpi_ora2enc_type(oratype)) {
            case ENC_TYPE_CHAR:
                CHK_STR_ENC(val, enc->enc);
                break;
            case ENC_TYPE_NCHAR:
                CHK_STR_ENC(val, enc->nenc);
                break;
            case ENC_TYPE_OTHER:
                SafeStringValue(val);
                break;
            }
            data->value.asBytes.ptr = RSTRING_PTR(val);
            data->value.asBytes.length = RSTRING_LEN(val);
            break;
        case DPI_NATIVE_TYPE_TIMESTAMP:
            rbdpi_to_dpiTimestamp(&data->value.asTimestamp, val);
            break;
        case DPI_NATIVE_TYPE_INTERVAL_DS:
            rbdpi_to_dpiIntervalDS(&data->value.asIntervalDS, val);
            break;
        case DPI_NATIVE_TYPE_INTERVAL_YM:
            rbdpi_to_dpiIntervalYM(&data->value.asIntervalYM, val);
            break;
        case DPI_NATIVE_TYPE_LOB:
            data->value.asLOB = rbdpi_to_lob(val)->handle;
            break;
        case DPI_NATIVE_TYPE_OBJECT:
            data->value.asObject = rbdpi_to_object(val)->handle;
            break;
        case DPI_NATIVE_TYPE_STMT:
            data->value.asStmt = rbdpi_to_stmt(val)->handle;
            break;
        case DPI_NATIVE_TYPE_BOOLEAN:
            data->value.asBoolean = RTEST(val) ? 1 : 0;
            break;
        case DPI_NATIVE_TYPE_ROWID:
            data->value.asRowid = rbdpi_to_rowid(val)->handle;
            break;
        default:
            rb_raise(rb_eRuntimeError, "unknown native type %d", type);
        }
    }
    return val;
}
