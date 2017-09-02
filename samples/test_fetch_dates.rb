#-----------------------------------------------------------------------------
# test_fetch_dates.rb
#   Tests simple fetch of dates.
#-----------------------------------------------------------------------------
#
# This was ported from TestFetchDates.c in ODPI-C.
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
pool = ODPI::Pool.new($main_user, $main_password, $connect_string)
conn = pool.connection

# prepare and execute statement
stmt = conn.prepare("select * from TestTimestamps")
stmt.execute

col_labels = stmt.query_columns.map.with_index do |col, i|
  format("%-5s %-18s", (i == 0) ? "Rows:" : "", col.name)
end

# fetch rows
while row = stmt.fetch
  row.each_with_index do |col, i|
    puts "#{col_labels[i]} = #{row[i] || "null"}"
  end
end

# display description of each variable
puts "Display column metadata"
stmt.query_columns.each do |colinfo|
  type = colinfo.type_info
  puts "('#{colinfo.name}', #{type.oracle_type}, #{type.size_in_chars}, #{type.client_size_in_bytes}, #{type.precision}, #{type.scale}, #{colinfo.nullable?})"
end
stmt.close
conn.close
puts "Done."
