#-----------------------------------------------------------------------------
# test_fetch.rb
#   Tests simple fetch of numbers and strings.
#-----------------------------------------------------------------------------
#
# This was ported from TestFetch.c in ODPI-C.
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

rowid = nil

# prepare and execute statement
stmt = conn.prepare("select IntCol, StringCol, RawCol, rowid from TestStrings where IntCol > :intCol")
stmt.bind(:intCol, 7)
stmt.execute

# fetch rows
puts "Fetch rows with IntCol > #{stmt[:intCol]}"
while row = stmt.fetch
  puts "Row: Int = #{row[0]}, String = '#{row[1]}', Raw = '#{row[2]}', Rowid = '#{row[3]}'"
  rowid = row[3]
end
puts ""

# display description of each variable
puts "Display column metadata"
stmt.query_columns.each do |colinfo|
  type = colinfo.type_info
  puts "('#{colinfo.name}', #{type.oracle_type}, #{type.size_in_chars}, #{type.client_size_in_bytes}, #{type.precision}, #{type.scale}, #{colinfo.nullable?})"
end
puts ""
stmt.close

puts "Fetch rows with rowid = #{rowid}"

# prepare and execute statement to fetch by rowid
stmt = conn.prepare("select IntCol from TestStrings where rowid = :1")
stmt.bind(1, rowid)
stmt.execute

# fetch rows
while row = stmt.fetch
  puts "Row: Int = #{row[0]}"
end
stmt.close
conn.close

puts "Done."
