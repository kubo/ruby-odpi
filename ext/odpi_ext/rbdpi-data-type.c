/*
 * rbdpi-struct.c -- part of ruby-odpi
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
 * ------------------------------------------------------
 * The license of documentation comments in this file is same with
 * ODPI-C because most of them were copied from ODPI-C documents.
 * The ODPI-C license is:
 *
 * Copyright (c) 2016, 2017 Oracle and/or its affiliates.  All rights reserved.
 * This program is free software: you can modify it and/or redistribute it
 * under the terms of:
 *
 * (i)  the Universal Permissive License v 1.0 or at your option, any
 *      later version (http://oss.oracle.com/licenses/upl); and/or
 *
 * (ii) the Apache License v 2.0. (http://www.apache.org/licenses/LICENSE-2.0)
 *
 * ------------------------------------------------------
 */
#include "rbdpi.h"

static VALUE cDataType;

/*
 * Document-class: ODPI::Dpi::DataType
 *
 * This class represents Oracle data type information.
 *
 * @see ODPI::Dpi::ObjectAttr#type_info
 * @see ODPI::Dpi::ObjectType#element_type_info
 * @see ODPI::Dpi::QueryInfo#type_info
 */
static void data_type_mark(void *arg)
{
    data_type_t *dt = (data_type_t *)arg;

    rb_gc_mark(dt->owner);
    rb_gc_mark(dt->objtype);
}

static size_t data_type_memsize(const void *arg)
{
    return sizeof(data_type_t);
}

static const struct rb_data_type_struct data_type_data_type = {
    "ODPI::Dpi::DataType",
    {data_type_mark, RUBY_TYPED_DEFAULT_FREE, data_type_memsize,},
    NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
};
#define TO_DATA_TYPE(obj) ((data_type_t *)rb_check_typeddata(obj, &data_type_data_type))

static VALUE data_type_alloc(VALUE klass)
{
    data_type_t *dt;
    VALUE obj = TypedData_Make_Struct(klass, data_type_t, &data_type_data_type, dt);

    dt->owner = Qnil;
    dt->self = obj;
    dt->objtype = Qnil;
    return obj;
}

/*
 * @private
 */
static VALUE data_type_initialize_copy(VALUE self, VALUE other)
{
    *TO_DATA_TYPE(self) = *TO_DATA_TYPE(other);
    return self;
}

/*
 * @private
 */
static VALUE data_type_inspect(VALUE self)
{
    VALUE str = rb_str_buf_new2("<");

    rb_str_buf_append(str, rb_obj_class(self));
    rb_str_buf_cat_ascii(str, ": ");
    rb_str_buf_append(str, self);
    rb_str_buf_cat_ascii(str, ">");
    return str;
}

/*
 * Gets the string representation of the Oracle data type,
 * which could be put in DDL statements.
 *
 * @return [String]
 */
static VALUE data_type_to_s(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);
    const dpiDataTypeInfo *info = dt->info;
    VALUE str;

    switch (info->oracleTypeNum) {
    case DPI_ORACLE_TYPE_VARCHAR:
        str = rb_sprintf("VARCHAR2(%d)", info->dbSizeInBytes);
        break;
    case DPI_ORACLE_TYPE_NVARCHAR:
        str = rb_sprintf("NVARCHAR(%d)", info->sizeInChars);
        break;
    case DPI_ORACLE_TYPE_CHAR:
        str = rb_sprintf("CHAR(%d)", info->dbSizeInBytes);
        break;
    case DPI_ORACLE_TYPE_NCHAR:
        str = rb_sprintf("NCHAR(%d)", info->sizeInChars);
        break;
    case DPI_ORACLE_TYPE_ROWID:
        str = rb_usascii_str_new_cstr("ROWID");
        break;
    case DPI_ORACLE_TYPE_RAW:
        str = rb_usascii_str_new_cstr("RAW");
        break;
    case DPI_ORACLE_TYPE_NATIVE_FLOAT:
        str = rb_usascii_str_new_cstr("BINARY_FLOAT");
        break;
    case DPI_ORACLE_TYPE_NATIVE_DOUBLE:
        str = rb_usascii_str_new_cstr("BINARY_DOUBLE");
        break;
    case DPI_ORACLE_TYPE_NATIVE_INT:
        str = rb_usascii_str_new_cstr("native int?");
        break;
    case DPI_ORACLE_TYPE_NUMBER:
        switch (info->scale) {
        case -127:
            switch (info->precision) {
            case 0:
                str = rb_usascii_str_new_cstr("NUMBER");
                break;
            case 126:
                str = rb_usascii_str_new_cstr("FLOAT");
                break;
            default:
                str = rb_sprintf("FLOAT(%d)", info->precision);
            }
            break;
        case 0:
            if (info->precision == 0) {
                str = rb_usascii_str_new_cstr("NUMBER");
            } else {
                str = rb_sprintf("NUMBER(%d)", info->precision);
            }
            break;
        default:
            str = rb_sprintf("NUMBER(%d,%d)", info->precision, info->scale);
        }
        break;
    case DPI_ORACLE_TYPE_DATE:
        str = rb_usascii_str_new_cstr("DATE");
        break;
    case DPI_ORACLE_TYPE_TIMESTAMP:
        if (info->fsPrecision == 6) {
            str = rb_usascii_str_new_cstr("TIMESTAMP");
        } else {
            str = rb_sprintf("TIMESTAMP(%d)", info->fsPrecision);
        }
        break;
    case DPI_ORACLE_TYPE_TIMESTAMP_TZ:
        if (info->fsPrecision == 6) {
            str = rb_usascii_str_new_cstr("TIMESTAMP WITH TIME ZONE");
        } else {
            str = rb_sprintf("TIMESTAMP(%d) WITH TIME ZONE", info->fsPrecision);
        }
        break;
    case DPI_ORACLE_TYPE_TIMESTAMP_LTZ:
        if (info->fsPrecision == 6) {
            str = rb_usascii_str_new_cstr("TIMESTAMP WITH LOCAL TIME ZONE");
        } else {
            str = rb_sprintf("TIMESTAMP(%d) WITH LOCAL TIME ZONE", info->fsPrecision);
        }
        break;
    case DPI_ORACLE_TYPE_INTERVAL_DS:
        if (info->precision == 2) {
            str = rb_usascii_str_new_cstr("INTERVAL YEAR TO MONTH");
        } else {
            str = rb_sprintf("INTERVAL YEAR(%d) TO MONTH", info->precision);
        }
        break;
    case DPI_ORACLE_TYPE_INTERVAL_YM:
        if (info->precision == 2 && info->fsPrecision == 6) {
            str = rb_usascii_str_new_cstr("INTERVAL DAY TO SECOND");
        } else {
            str = rb_sprintf("INTERVAL DAY(%d) TO SECOND(%d)",
                              info->precision, info->fsPrecision);
        }
        break;
    case DPI_ORACLE_TYPE_CLOB:
        str = rb_usascii_str_new_cstr("CLOB");
        break;
    case DPI_ORACLE_TYPE_NCLOB:
        str = rb_usascii_str_new_cstr("NCLOB");
        break;
    case DPI_ORACLE_TYPE_BLOB:
        str = rb_usascii_str_new_cstr("BLOB");
        break;
    case DPI_ORACLE_TYPE_BFILE:
        str = rb_usascii_str_new_cstr("BFILE");
        break;
    case DPI_ORACLE_TYPE_STMT:
        str = rb_usascii_str_new_cstr("CURSOR");
        break;
    case DPI_ORACLE_TYPE_BOOLEAN:
        str = rb_usascii_str_new_cstr("BOOLEAN");
        break;
    case DPI_ORACLE_TYPE_OBJECT:
        if (!NIL_P(dt->objtype) && dt->enc.enc != NULL) {
            object_type_t *objtype = rbdpi_to_object_type(dt->objtype);
            str = rb_enc_sprintf(dt->enc.enc, "%.*s.%.*s",
                                 objtype->info.schemaLength, objtype->info.schema,
                                 objtype->info.nameLength, objtype->info.name);
        } else {
            str = rb_usascii_str_new_cstr("OBJECT");
        }
        break;
    case DPI_ORACLE_TYPE_LONG_VARCHAR:
        str = rb_usascii_str_new_cstr("LONG");
        break;
    case DPI_ORACLE_TYPE_LONG_RAW:
        str = rb_usascii_str_new_cstr("LONG RAW");
        break;
    case DPI_ORACLE_TYPE_NATIVE_UINT:
        str = rb_usascii_str_new_cstr("native uint?");
        break;
    default:
        str = rb_sprintf("Unknown oracle type number %d",
                         info->oracleTypeNum);
    }
    if (RB_OBJ_TAINTED(self)) {
        RB_OBJ_TAINT(str);
    }
    return str;
}

/*
 * Gets the type of the data. +nil+ if the type is not supported by
 * ODPI-C.
 *
 * @return [Symbol]
 */
static VALUE data_type_get_oracle_type(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);

    return rbdpi_from_dpiOracleTypeNum(dt->info->oracleTypeNum);
}

/*
 * Gets the default native type for the data. +nil+ if the type is not
 * supported by ODPI-C.
 *
 * @return [Symbol]
 */
static VALUE data_type_get_default_native_type(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);

    return rbdpi_from_dpiNativeTypeNum(dt->info->defaultNativeTypeNum);
}

/*
 * Get the OCI type code for the data, which can be useful if the type
 * is not supported by ODPI-C.
 *
 * @return [Integer]
 */
static VALUE data_type_get_oci_type_code(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);

    return INT2FIX(dt->info->ociTypeCode);

}

/*
 * Get the size in bytes (from the database's perspective) of the data.
 * This value is only for strings and binary data. For all other
 * data the value is zero.
 *
 * @return [Integer]
 */
static VALUE data_type_get_db_size_in_bytes(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);

    return UINT2NUM(dt->info->dbSizeInBytes);
}

/*
 * Gets the size in bytes (from the client's perspective) of the data.
 * This value is only for strings and binary data. For all other
 * data the value is zero.
 *
 * @return [Integer]
 */
static VALUE data_type_get_client_size_in_bytes(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);

    return UINT2NUM(dt->info->clientSizeInBytes);
}

/*
 * Gets the size in characters of the data. This value is only
 * for string data. For all other data the value is zero.
 *
 * @return [Integer]
 */
static VALUE data_type_get_size_in_chars(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);

    return UINT2NUM(dt->info->sizeInChars);
}

/*
 * Gets the precision of the data. This value is only for
 * numeric and interval data. For all other data the value is zero.
 *
 * @return [Integer]
 */
static VALUE data_type_get_precision(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);

    return INT2FIX(dt->info->precision);
}

/*
 * Gets the scale of the data. This value is only for numeric
 * data. For all other data the value is zero.
 *
 * @return [Integer]
 */
static VALUE data_type_get_scale(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);

    return INT2FIX(dt->info->scale);
}

/*
 * Gets the fractional seconds precision of the data. This value is only
 * for timestamp and interval day to second data. For all other
 * data the value is zero.
 *
 * @return [Integer]
 */
static VALUE data_type_get_fs_precision(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);

    return INT2FIX(dt->info->fsPrecision);
}

/*
 * Gets the type of the object. This value is only for named type
 * data. For all other data the value is +nil+. This
 *
 * @return [ObjectType]
 */
static VALUE data_type_get_object_type(VALUE self)
{
    data_type_t *dt = TO_DATA_TYPE(self);

    return dt->objtype;
}

void Init_rbdpi_data_type(VALUE mDpi)
{
    cDataType = rb_define_class_under(mDpi, "DataType", rb_cObject);
    rb_define_alloc_func(cDataType, data_type_alloc);
    rb_define_method(cDataType, "initialize", rbdpi_initialize_error, -1);
    rb_define_method(cDataType, "initialize_copy", data_type_initialize_copy, 1);
    rb_define_method(cDataType, "inspect", data_type_inspect, 0);
    rb_define_method(cDataType, "to_s", data_type_to_s, 0);
    rb_define_method(cDataType, "oracle_type", data_type_get_oracle_type, 0);
    rb_define_method(cDataType, "default_native_type", data_type_get_default_native_type, 0);
    rb_define_method(cDataType, "oci_type_code", data_type_get_oci_type_code, 0);
    rb_define_method(cDataType, "db_size_in_bytes", data_type_get_db_size_in_bytes, 0);
    rb_define_method(cDataType, "client_size_in_bytes", data_type_get_client_size_in_bytes, 0);
    rb_define_method(cDataType, "size_in_chars", data_type_get_size_in_chars, 0);
    rb_define_method(cDataType, "precision", data_type_get_precision, 0);
    rb_define_method(cDataType, "scale", data_type_get_scale, 0);
    rb_define_method(cDataType, "fs_precision", data_type_get_fs_precision, 0);
    rb_define_method(cDataType, "object_type", data_type_get_object_type, 0);
}

VALUE rbdpi_from_dpiDataTypeInfo(const dpiDataTypeInfo *info, VALUE owner, const rbdpi_enc_t *enc)
{
    data_type_t *dt;
    VALUE obj = TypedData_Make_Struct(cDataType, data_type_t, &data_type_data_type, dt);

    if (NIL_P(owner)) {
        dpiDataTypeInfo *buf;

        owner = rb_str_buf_new(sizeof(*info));
        buf = (dpiDataTypeInfo *)RSTRING_PTR(owner);
        *buf = *info;
        info = buf;
    }

    dt->info = info;
    dt->owner = owner;
    dt->self = obj;
    if (info->objectType != NULL) {
        dt->objtype = rbdpi_from_object_type(info->objectType, enc);
        dpiObjectType_addRef(info->objectType);
    } else {
        dt->objtype = Qnil;
    }
    dt->enc = *enc;
    return obj;
}

const data_type_t *rbdpi_to_data_type(VALUE obj)
{
    data_type_t *dt = (data_type_t *)rb_check_typeddata(obj, &data_type_data_type);

    if (dt->info == NULL) {
        rb_raise(rb_eRuntimeError, "%s isn't initialized", rb_obj_classname(obj));
    }
    return dt;
}
