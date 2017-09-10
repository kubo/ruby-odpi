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

    class Base < Dpi::Var

      def self.create(*args)
        self.new(*args)
      end

      def initialize(conn, oracle_type, native_type, array_size, size, size_is_bytes, is_array, objtype)
        @is_array = is_array
        super
      end

      def get
        if @is_array
          len = self.num_elements_in_array
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
          len = val.length
          self.num_elements_in_array = len
          0.upto(len - 1) do |idx|
            self[idx] = val[idx]
          end
        else
          self[0] = val
        end
        self
      end

      def convert_in(val)
        val
      end

      def convert_out(val)
        val
      end
    end

    class BinaryDouble < Base
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, :native_double, :double, array_size, 0, false, is_array, nil)
      end
    end

    class BinaryFloat < Base
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, :native_float, :float, array_size, 0, false, is_array, nil)
      end
    end

    class TimestampBase < Base
      @@datetime_fsec_base = (1 / ::DateTime.parse('0001-01-01 00:00:00.000000001').sec_fraction).to_i

      def convert_in(val)
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

      def convert_out(val)
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
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, :date, :timestamp, array_size, 0, false, is_array, nil)
      end
    end

    class Timestamp < TimestampBase
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, :timestamp, :timestamp, array_size, 0, false, is_array, nil)
      end
    end

    class TimestampTZ < TimestampBase
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, :timestamp_tz, :timestamp, array_size, 0, false, is_array, nil)
      end
    end

    class TimestampLTZ < TimestampBase
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, :timestamp_ltz, :timestamp, array_size, 0, false, is_array, nil)
      end
    end

    class Float < Base
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, :number, :bytes, array_size, 0, false, is_array, nil)
      end

      def convert_in(val)
        val.to_f.to_s
      end

      def convert_out(val)
        val.to_f
      end
    end

    class Integer < Base
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, :number, :bytes, array_size, 0, false, is_array, nil)
      end

      def convert_in(val)
        val.to_i.to_s
      end

      def convert_out(val)
        val.to_i
      end
    end

    class Int64 < Base
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, :number, :int64, array_size, 0, false, is_array, nil)
      end
    end

    class Number
      def self.create(conn, value, type, params, array_size, is_array)
        if params.scale == 0
          prec = params.precision
          if prec == 0
            Float.new(conn, value, type, params, array_size, is_array)
          elsif prec < 19
            Int64.new(conn, value, type, params, array_size, is_array)
          else
            Integer.new(conn, value, type, params, array_size, is_array)
          end
        else
          Float.new(conn, value, type, params, array_size, is_array)
        end
      end
    end

    class Raw < Base
      def initialize(conn, value, type, params, array_size, is_array)
        if params.is_a? Hash
          size = params[:length]
          size = value.bytesize if size.nil?
        else
          size = params.client_size_in_bytes
        end
        super(conn, :raw, :bytes, array_size, size, true, is_array, nil)
      end
    end

    class Rowid < Base
      def initialize(conn, value, type, params, array_size, is_array)
        super(conn, :rowid, :rowid, array_size, 0, false, is_array, nil)
      end
    end

    class String < Base
      def initialize(conn, value, type, params, array_size, is_array)
        if params.is_a? Hash
          size = params[:length]
          size = value.size if size.nil?
        else
          size = params.size_in_chars
        end
        super(conn, :varchar, :bytes, array_size, size, true, is_array, nil)
      end
    end

    class Object < Base
      def initialize(conn, value, type, params, array_size, is_array)
        @conn = conn
        @objtype = params.object_type
        super(conn, :object, :object, array_size, 0, nil, is_array, @objtype)
      end

      def convert_in(val)
        val.__object__
      end

      def convert_out(val)
        klass = ODPI::Object.find_class(@objtype.schema, @objtype.name)
        klass.new(@conn, @objtype, val)
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
