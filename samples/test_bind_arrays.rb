#-----------------------------------------------------------------------------
# test_bind_arrays.rb
#   Tests calling stored procedures binding PL/SQL arrays in various ways.
#-----------------------------------------------------------------------------
#
# This was ported from TestBindArrays.c in ODPI-C.
# The following is the original copyright:
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
# Copyright (c) 2016, 2017 Oracle and/or its affiliates.  All rights reserved.
# This program is free software: you can modify it and/or redistribute it
# under the terms of:
#
# (i)  the Universal Permissive License v 1.0 or at your option, any
#      later version (http://oss.oracle.com/licenses/upl); and/or
#
# (ii) the Apache License v 2.0. (http://www.apache.org/licenses/LICENSE-2.0)
#-----------------------------------------------------------------------------

require 'odpi'
require File.join(File.dirname(File.absolute_path(__FILE__)), 'config.rb')

# connect to database
conn = ODPI::connect($main_user, $main_password, $connect_string)

array_var = ["Test String 1 (I)",
             "Test String 2 (II)",
             "Test String 3 (III)",
             "Test String 4 (IV)",
             "Test String 5 (V)"]

# ************** IN ARRAYS *****************
# prepare statement for testing in arrays
stmt = conn.prepare('begin :1 := pkg_TestStringArrays.TestInArrays(:2, :3); end;')
stmt.bind(1, nil, Integer)
stmt.bind(2, 12)
stmt.bind(3, array_var)

# perform execution (in arrays with 5 elements)
stmt.execute

puts "IN array (5 elements): return value is #{stmt[1]}"
puts
stmt.close

# ************** IN/OUT ARRAYS *****************
# prepare statement for testing in/out arrays
stmt = conn.prepare('begin pkg_TestStringArrays.TestInOutArrays(:1, :2); end;')
stmt.bind(1, 5)
stmt.bind(2, array_var, [String], max_array_size: 8, length: 60)

# perform execution (in/out arrays)
stmt.execute

# display value of array after procedure call
puts 'IN/OUT array contents:'
stmt[2].each_with_index do |val, idx|
  puts "    [#{idx}] #{val}"
end
puts
stmt.close

# ************** OUT ARRAYS *****************
# prepare statement for testing out arrays
stmt = conn.prepare('begin pkg_TestStringArrays.TestOutArrays(:1, :2); end;')
stmt.bind(1, 7)
stmt.bind(2, nil, [String], length: 60, max_array_size: 8)

# perform execution (out arrays)
stmt.execute

# display value of array after procedure call
puts 'OUT array contents:'
stmt[2].each_with_index do |val, idx|
  puts "    [#{idx}] #{val}"
end
puts
stmt.close

#define SQL_ASSOC ""

# ************** INDEX-BY ASSOCIATIVE ARRAYS *****************
# look up object type by name
class UdtStringlist < ODPI::Object::Base
  set_typename 'PKG_TESTSTRINGARRAYS.UDT_STRINGLIST'
end

stmt = conn.prepare('begin pkg_TestStringArrays.TestIndexBy(:1); end;')
stmt.bind(1, nil, UdtStringlist)

# perform execution (associative arrays)
stmt.execute

# display contents of array after procedure call
puts 'ASSOCIATIVE array contents:'

obj = stmt[1]
obj.to_hash.each do |key, val|
  puts "    [#{key}] #{val}"
end
puts
stmt.close

# clean up
conn.close

puts "Done."
