#-----------------------------------------------------------------------------
# test_fetch_objects.rb
#   Tests simple fetch of objects.
#-----------------------------------------------------------------------------
#
# This was ported from TestFetchObjects.c in ODPI-C.
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

class UdtObject < ODPI::Object::Base
end

# connect to database
pool = ODPI::Pool.new($main_user, $main_password, $connect_string)
conn = pool.connection

# prepare and execute statement
stmt = conn.prepare("select ObjectCol from TestObjects order by IntCol")
stmt.execute

# get object type and attributes
objtype = stmt.query_columns[0].type_info.object_type
puts "Fetching objects of type #{objtype.schema}.#{objtype.name}"

# fetch rows
while row = stmt.fetch
  col = row[0]
  p col
  if col.nil?
    puts "Row: ObjCol = null"
  else
    puts "Row: ObjCol ="
    puts "    NumberValue => #{col.numbervalue}"
    puts "    StringValue => #{col.stringvalue}"
    puts "    FixedCharValue => #{col.fixedcharvalue}"
    puts "    DateValue => #{col.datevalue}"
    puts "    TimestampValue => #{col.timestampvalue}"
    puts "    SubObjectValue => #{col.subobjectvalue}"
    puts "    SubObjectArray => #{col.subobjectarray}"
  end
end

stmt.close
conn.close

puts "Done"
