# object.rb -- part of ruby-odpi
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

  module Object
    @@oracle_type_to_ruby_class = {}
    @@ruby_class_to_oracle_type = {}

    def self.oracle_name_to_ruby_constant_name(name)
      name = if name !~ /[[:lower:]]/
               name.downcase.gsub(/(^|_)([[:lower:]])/) { $2.upcase }
             else
               name.sub(/^([a-z])/) { $1.upcase }
             end.gsub(/[$#]/, '_')
      if name =~ /^[A-Z]/
        name
      else
        'C' + name
      end
    end

    def self.oracle_name_to_ruby_identifier_name(name)
      if name !~ /[[:lower:]]/
        name.downcase
      else
        name.sub(/^([a-z])/) { $1.downcase }
      end.gsub(/[$#]/, '_')
    end

    def self.find_class(schema, name)
      fullname = "#{schema}.#{name}"
      klass = @@oracle_type_to_ruby_class[fullname]
      klass = @@oracle_type_to_ruby_class[name] if klass.nil?
      if klass.nil?
        module_name = oracle_name_to_ruby_constant_name(schema)
        class_name = oracle_name_to_ruby_constant_name(name)

        if self.const_defined?(module_name, false)
          mod = self.const_get(module_name, false)
        else
          mod = self.const_set(module_name, Module.new)
        end

        if mod.const_defined?(class_name, false)
          klass = mod.const_get(class_name, false)
        else
          klass = mod.const_set(class_name, Class.new(Base))
        end
        @@oracle_type_to_ruby_class[fullname] = klass
        @@ruby_class_to_oracle_type[klass] = fullname
      end
      klass
    end

    def self.find_name_by_class(klass)
      @@ruby_class_to_oracle_type[klass]
    end

    def self.set_typename(klass, name)
      @@ruby_class_to_oracle_type[klass] = name
      @@oracle_type_to_ruby_class[name] = klass
    end

    module NamedCollection
      def indexes
        ary = []
        idx = @object.first_index
        while idx
          ary << idx
          idx = @object.next_index(idx)
        end
        ary
      end

      def to_hash
        hash = {}
        idx = @object.first_index
        while idx
          hash[idx] = self[idx]
          idx = @object.next_index(idx)
        end
        hash
      end

      def to_ary
        ary = []
        idx = @object.first_index
        while idx
          ary.push(self[idx])
          idx = @object.next_index(idx)
        end
        ary
      end

      def [](idx)
        val = @object.element_value_by_index(idx, @bindclass::TYPES[1])
        val.nil? ? nil : @bindclass.convert_out(@conn, val)
      end

      def []=(idx, val)
        val = val.nil? ? nil : @bindclass.convert_in(@conn, val)
        @object.set_element_value_by_index(idx, @bindclass::TYPES[1], val)
      end

      def <<(val)
        val = val.nil? ? nil : @bindclass.convert_in(@conn, val)
        @object.append_element(val, @bindclass::TYPES[1])
      end

      def delete_at(idx)
        @object.delete_element_by_index(idx)
      end

      def exists_at?(idx)
        @object.element_exists_by_index?(idx)
      end

      def trim(num)
        @object.trim(num)
      end
    end

    class Base
      def self.inherited(klass)
        class_name = klass.to_s
        if class_name !~ /#<Class:/
          name = klass.to_s.gsub(/^.*::/, '').gsub(/([a-z\d])([A-Z])/,'\1_\2').upcase
          ODPI::Object.set_typename(klass, name)
        end
      end

      def self.set_typename(name)
        ODPI::Object.set_typename(self, name)
      end

      def initialize(conn, objtype = nil, object = nil)
        @conn = conn
        @objtype = objtype || @conn.raw_connection.object_type(ODPI::Object.find_name_by_class(self.class))
        @object = object || @objtype.create_object
        if @objtype.is_collection?
          self.extend(NamedCollection)
          @attr_types = nil
          @elem_type = @objtype.element_type_info
          @bindclass = ODPI::BindType::ObjectAttrMapping[@elem_type.oracle_type].to_bindclass(@elem_type)
        else
          @attr_types = {}
          @objtype.attributes.each do |attr|
            attr_name = ODPI::Object.oracle_name_to_ruby_identifier_name(attr.name).intern
            @attr_types[attr_name] = attr
          end
          @elem_type = nil
        end
      end

      def method_missing(method_id, *args)
        unless @objtype.is_collection?
          if @attr_types.has_key?(method_id)
            self.class.class_eval do
              define_method(method_id) do
                attr = @attr_types[method_id]
                bindclass = ODPI::BindType::ObjectAttrMapping[attr.type_info.oracle_type].to_bindclass(attr.type_info)
                val = @object.attribute_value(attr, bindclass::TYPES[1])
                val.nil? ? nil : bindclass.convert_out(@conn, val)
              end
            end
            return self.send(method_id, *args)
          elsif method_id[-1] == '=' && @attr_types.has_key?(attr_name = method_id[0..-2].intern)
            self.class.class_eval do
              define_method(method_id) do |val|
                attr = @attr_types[attr_name]
                bindclass = ODPI::BindType::ObjectAttrMapping[attr.type_info.oracle_type].to_bindclass(attr.type_info)
                val = val.nil? ? nil : bindclass.convert_in(@conn, val)
                @object.set_attribute_value(attr, bindclass::TYPES[1], val)
              end
            end
            return self.send(method_id, *args)
          end
        end
        super(method_id, *args)
      end

      def to_s
        attrs = if @attr_types
                  @attr_types.keys.collect do |name| "#{name}: #{self.send(name).inspect}" end
                else
                  ary = self.to_hash.map do |key, val|
                    "#{key}: #{val.inspect}"
                  end
                  if ary.length > 4
                    ary[0..4] << '...'
                  else
                    ary
                  end
                end
        %Q[#<#{self.class}: #{@objtype.schema}.#{@objtype.name} (#{attrs.join(', ')})>]
      end

      def inspect
        self.to_s
      end

      def __attr_names__
        @attr_types.keys
      end

      def __object__
        @object
      end
    end
  end
end
