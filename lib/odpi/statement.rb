# statement.rb -- part of ruby-odpi
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

module ODPI
  class Statement
    def initialize(conn, stmt)
      @conn = conn
      @stmt = stmt
      @column_vars = []
      @column_info = nil
      @bind_vars = {}
      @executed = false
    end

    def query?
      @stmt.query?
    end

    def plsql?
      @stmt.plsql?
    end

    def ddl?
      @stmt.ddl?
    end

    def dml?
      @stmt.dml?
    end

    def type
      @stmt.statement_type
    end

    def returning?
      @stmt.returning?
    end

    def bind_names
      @stmt.bind_names
    end

    def fetch_array_size
      @stmt.fetch_array_size
    end

    def fetch_array_size=(size)
      @stmt.fetch_array_size = size
    end

    def bind(key, value, type = nil, params = {})
      var = make_var(value, type, params, nil)
      if key.is_a? Integer
        @stmt.bind_by_pos(key, var.raw_var)
      else
        @stmt.bind_by_name(key, var.raw_var)
      end
      var.set(value)
      @bind_vars[key] = var
      self
    end

    def [](key)
      @bind_vars[key].get
    end

    def []=(key, val)
      @bind_vars[key].set(val)
    end

    def define(pos, type, params = {})
      var = make_var(nil, type, params)
      @stmt.define(pos, var) if @executed
      @column_vars[pos - 1] = var
      self
    end

    def query_columns
      @stmt.query_columns
    end

    def execute(binds = nil, defines = nil, params = nil)
      @stmt.execute(:default)
      if @stmt.query?
        @stmt.query_columns.each_with_index do |col, idx|
          unless @column_vars[idx]
            type = col.type_info
            @column_vars[idx] = make_var(nil, type.oracle_type, type, @stmt.fetch_array_size)
          end
        end
        unless @executed
          @column_vars.each_with_index do |var, idx|
            @stmt.define(idx + 1, var.raw_var)
          end
        end
      end
      @executed = true
      self
    end

    def fetch
      idx = @stmt.fetch
      if idx
        @column_vars.collect do |var|
          var[idx]
        end
      end
    end

    def close
      @stmt.close(nil)
    end

    private

    def make_var(value, type, params, array_size)
      is_array = false
      array_size ||= params[:max_array_size]
      if type.nil?
        type = value.class
        if type == Array
          type = value[0].class
          is_array = true
          array_size ||= value.length
        end
      elsif type.is_a? Array
        type = type[0]
        is_array = true
      end
      array_size ||= 1
      if type.is_a?(Class) && type < ODPI::Object::Base
        bind_class = BindType::Mapping[:object]
      else
        bind_class = BindType::Mapping[type].to_bindclass(params)
      end
      raise "Unsupported bind type: #{type}" if bind_class.nil?
      bind_class.new(@conn, value, type, params, array_size, is_array)
    end
  end
end
