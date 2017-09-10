# bindtype.rb -- part of ruby-odpi
#
# URL: https://github.com/kubo/ruby-odpi
#
# ------------------------------------------------------
#
# Copyright 2017 Kubo Takehiro <kubo@jiubao.org>
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
#    1. Redistributions of source code must retain the above copyright notice, this list of
#       conditions and the following disclaimer.
#
#    2. Redistributions in binary form must reproduce the above copyright notice, this list
#       of conditions and the following disclaimer in the documentation and/or other materials
#       provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# The views and conclusions contained in the software and documentation are those of the
# authors and should not be interpreted as representing official policies, either expressed
# or implied, of the authors.
require 'date'

module ODPI

  module BindType
    Mapping = {}
    ObjectAttrMapping = {}

    class Base

      attr_reader :raw_var

      def self.to_bindclass(params)
        self
      end

      def initialize(conn, array_size, size, size_is_bytes, is_array, objtype)
        @conn = conn
        @is_array = is_array
        oracle_type, native_type = self.class::TYPES
        @raw_var = Dpi::Var.new(conn, oracle_type, native_type, array_size, size, size_is_bytes, is_array, objtype)
      end

      def get
        if @is_array
          len = @raw_var.num_elements_in_array
          ary = Array.new(len)
          0.upto(len - 1) do |idx|
            ary[idx] = self[idx]
          end
          ary
        else
          self[0]
        end
      end

      def set(val)
        if @is_array
          if val.nil?
            @raw_var.num_elements_in_array = 0
          else
            len = val.length
            @raw_var.num_elements_in_array = len
            0.upto(len - 1) do |idx|
              self[idx] = val[idx]
            end
          end
        else
          self[0] = val
        end
        self
      end

      def []=(key, val)
        @raw_var[key] = val.nil? ? nil : self.class.convert_in(@conn, val)
      end

      def [](key)
        val = @raw_var[key]
        val.nil? ? nil : self.class.convert_out(@conn, val)
      end

      def self.convert_in(conn, val)
        val
      end

      def self.convert_out(conn, val)
        val
      end
    end

    class BinaryDouble < Base
      TYPES = [:native_double, :double]
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, array_size, 0, false, is_array, nil)
      end
    end

    class BinaryFloat < Base
      TYPES = [:native_float, :float]
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, array_size, 0, false, is_array, nil)
      end
    end

    class TimestampBase < Base
      @@datetime_fsec_base = (1 / ::DateTime.parse('0001-01-01 00:00:00.000000001').sec_fraction).to_i

      def self.convert_in(conn, val)
        # year
        year = val.year
        # month
        if val.respond_to? :mon
          month = val.mon
        elsif val.respond_to? :month
          month = val.month
        else
          raise "expect Time, Date or DateTime but #{val.class}"
        end
        # day
        if val.respond_to? :mday
          day = val.mday
        elsif val.respond_to? :day
          day = val.day
        else
          raise "expect Time, Date or DateTime but #{val.class}"
        end
        # hour
        if val.respond_to? :hour
          hour = val.hour
        else
          hour = 0
        end
        # minute
        if val.respond_to? :min
          minute = val.min
        else
          minute = 0
        end
        # second
        if val.respond_to? :sec
          sec = val.sec
        else
          sec = 0
        end
        # fractional second
        if val.respond_to? :sec_fraction
          fsec = (val.sec_fraction * @@datetime_fsec_base).to_i
        elsif val.respond_to? :nsec
          fsec = val.nsec
        elsif val.respond_to? :usec
          fsec = val.usec * 1000
        else
          fsec = 0
        end
        # time zone
        if val.respond_to? :offset
          # DateTime
          tz_min = (val.offset * 1440).to_i
        elsif val.respond_to? :utc_offset
          # Time
          tz_min = val.utc_offset / 60
        else
          tz_hour = nil
          tz_min = nil
        end
        if tz_min
          if tz_min < 0
            tz_min = - tz_min
            tz_hour = - (tz_min / 60)
            tz_min = (tz_min % 60)
          else
            tz_hour = tz_min / 60
            tz_min = tz_min % 60
          end
        end
        [year, month, day, hour, minute, sec, fsec, tz_hour, tz_min]
      end

      def self.convert_out(conn, val)
        year, month, day, hour, minute, sec, nsec, tz_hour, tz_min = val

        sec += nsec.to_r / 1_000_000_000 if nsec && nsec != 0
        if tz_hour
          utc_offset = tz_hour * 3600 + tz_min * 60
          ::Time.new(year, month, day, hour, minute, sec, utc_offset)
        else
          ::Time.utc(year, month, day, hour, minute, sec) # TODO: add a parameter to use Time.local.
        end
      end
    end

    class Date < TimestampBase
      TYPES = [:date, :timestamp]
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, array_size, 0, false, is_array, nil)
      end
    end

    class Timestamp < TimestampBase
      TYPES = [:timestamp, :timestamp]
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, array_size, 0, false, is_array, nil)
      end
    end

    class TimestampTZ < TimestampBase
      TYPES = [:timestamp_tz, :timestamp]
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, array_size, 0, false, is_array, nil)
      end
    end

    class TimestampLTZ < TimestampBase
      TYPES = [:timestamp_ltz, :timestamp]
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, array_size, 0, false, is_array, nil)
      end
    end

    class Float < Base
      TYPES = [:number, :bytes]
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, array_size, 0, false, is_array, nil)
      end

      def self.convert_in(conn, val)
        val.to_f.to_s
      end

      def self.convert_out(conn, val)
        val.to_f
      end
    end

    class Integer < Base
      TYPES = [:number, :bytes]
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, array_size, 0, false, is_array, nil)
      end

      def self.convert_in(conn, val)
        val.to_i.to_s
      end

      def self.convert_out(conn, val)
        val.to_i
      end
    end

    class Int64 < Base
      TYPES = [:number, :int64]
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, array_size, 0, false, is_array, nil)
      end
    end

    class Number
      def self.to_bindclass(params)
        if params.scale == 0
          prec = params.precision
          if prec == 0
            Float
          elsif prec < 19
            Int64
          else
            Integer
          end
        else
          Float
        end
      end
    end

    class Raw < Base
      TYPES = [:raw, :bytes]
      def initialize(conn, value, type, params, array_size, is_array)
        if params.is_a? Hash
          size = params[:length]
          if size.nil?
            if is_array
              size = value.collect(&:bytesize).max
            else
              size = value.bytesize
            end
          end
        else
          size = params.client_size_in_bytes
        end
        super(conn, array_size, size, true, is_array, nil)
      end
    end

    class Rowid < Base
      TYPES = [:rowid, :rowid]
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, array_size, 0, false, is_array, nil)
      end
    end

    class String < Base
      TYPES = [:varchar, :bytes]
      def initialize(conn, value, type, params, array_size, is_array)
        if params.is_a? Hash
          size = params[:length]
          if size.nil?
            if is_array
              size = value.collect(&:size).max
            else
              size = value.size
            end
          end
        else
          size = params.size_in_chars
        end
        super(conn, array_size, size, true, is_array, nil)
      end
    end

    class Object < Base
      TYPES = [:object, :object]
      def initialize(conn, value, type, params, array_size, is_array)
        if type.is_a?(Class) && type < ODPI::Object::Base
          objtype = conn.object_type(ODPI::Object.find_name_by_class(type))
        else
          objtype = params.object_type
        end
        super(conn, array_size, 0, nil, is_array, objtype)
      end

      def self.convert_in(conn, val)
        val.__object__
      end

      def self.convert_out(conn, val)
        objtype = val.object_type
        klass = ODPI::Object.find_class(objtype.schema, objtype.name)
        klass.new(conn, objtype, val)
      end
    end
  end
end

ODPI::BindType::Mapping[Bignum] = ODPI::BindType::Integer if 0.class != Integer
ODPI::BindType::Mapping[Float] = ODPI::BindType::Float
ODPI::BindType::Mapping[Fixnum] = ODPI::BindType::Integer if 0.class != Integer
ODPI::BindType::Mapping[Integer] = ODPI::BindType::Integer
ODPI::BindType::Mapping[String] = ODPI::BindType::String
ODPI::BindType::Mapping[ODPI::Dpi::Rowid] = ODPI::BindType::Rowid
ODPI::BindType::Mapping[Time] = ODPI::BindType::TimestampTZ

ODPI::BindType::Mapping[:varchar] = ODPI::BindType::String
ODPI::BindType::Mapping[:nvarchar] = ODPI::BindType::String
ODPI::BindType::Mapping[:char] = ODPI::BindType::String
ODPI::BindType::Mapping[:nchar] = ODPI::BindType::String
ODPI::BindType::Mapping[:rowid] = ODPI::BindType::Rowid
ODPI::BindType::Mapping[:raw] = ODPI::BindType::Raw
ODPI::BindType::Mapping[:native_double] = ODPI::BindType::BinaryDouble
ODPI::BindType::Mapping[:native_float] = ODPI::BindType::BinaryFloat
ODPI::BindType::Mapping[:number] = ODPI::BindType::Number
ODPI::BindType::Mapping[:date] = ODPI::BindType::Date
ODPI::BindType::Mapping[:timestamp] = ODPI::BindType::Timestamp
ODPI::BindType::Mapping[:timestamp_tz] = ODPI::BindType::TimestampTZ
ODPI::BindType::Mapping[:timestamp_ltz] = ODPI::BindType::TimestampLTZ
ODPI::BindType::Mapping[:object] = ODPI::BindType::Object

ODPI::BindType::ObjectAttrMapping[:varchar] = ODPI::BindType::String
ODPI::BindType::ObjectAttrMapping[:nvarchar] = ODPI::BindType::String
ODPI::BindType::ObjectAttrMapping[:char] = ODPI::BindType::String
ODPI::BindType::ObjectAttrMapping[:nchar] = ODPI::BindType::String
ODPI::BindType::ObjectAttrMapping[:rowid] = ODPI::BindType::Rowid
ODPI::BindType::ObjectAttrMapping[:raw] = ODPI::BindType::Raw
ODPI::BindType::ObjectAttrMapping[:native_double] = ODPI::BindType::BinaryDouble
ODPI::BindType::ObjectAttrMapping[:native_float] = ODPI::BindType::BinaryFloat
ODPI::BindType::ObjectAttrMapping[:number] = ODPI::BindType::BinaryDouble # ODPI::BindType::Number
ODPI::BindType::ObjectAttrMapping[:date] = ODPI::BindType::Date
ODPI::BindType::ObjectAttrMapping[:timestamp] = ODPI::BindType::Timestamp
ODPI::BindType::ObjectAttrMapping[:timestamp_tz] = ODPI::BindType::TimestampTZ
ODPI::BindType::ObjectAttrMapping[:timestamp_ltz] = ODPI::BindType::TimestampLTZ
ODPI::BindType::ObjectAttrMapping[:object] = ODPI::BindType::Object
