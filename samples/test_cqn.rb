#-----------------------------------------------------------------------------
# test_cqn.rb
#   Tests continuous query notification.
#-----------------------------------------------------------------------------
#
# This was ported from Test_CQN.c in ODPI-C.
# The following is the original copyright:
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

require 'odpi_ext'
require File.join(File.dirname(__FILE__), 'config.rb')

#-----------------------------------------------------------------------------
# test_callback
#   Test callback for continuous query notification.
#-----------------------------------------------------------------------------
test_callback = Proc.new do |message|
  if message.error_info
    errinfo = message.error_info.message
    STDERR.puts "ERROR: #{errinfo.message} (#{errinfo.fn_name}: #{errinfo.action}"
    return
  end

  # display contents of message
  puts "==========================================================="
  puts "NOTIFICATION RECEIVED from database #{message.db_name} (SUBSCR ID #{message.event_type})"
  puts "==========================================================="
  message.queries.each do |query|
    puts "--> Query ID: #{query.id}"
    query.tables.each do |table|
      puts "--> --> Table Name: #{table.name}"
      puts "--> --> Table Operation: #{table.operation}"
      if table.rows
        puts "--> --> Table Rows:"
        table.rows.each do |row|
          puts "--> --> --> ROWID: #{row.rowid}"
          puts "--> --> --> Operation: #{row.operation}"
        end
      end
    end
  end
end

#-----------------------------------------------------------------------------
# main
#-----------------------------------------------------------------------------

# connect to database
# NOTE: events mode must be configured
create_params = ODPI::Dpi::CommonCreateParams.new
create_params.events = true
conn = ODPI::Dpi::Conn.new($main_user, $main_password, $connect_string, create_params, nil)

# create subscription
params = ODPI::Dpi::SubscrCreateParams.new
params.qos = [:query, :rowids]
params.callback = test_callback
subscr = conn.new_subscription(params)

# register query
sql_text = "select * from TestTempTable"
stmt = subscr.prepare_stmt(sql_text)
stmt.execute(nil)
query_id = stmt.subscr_query_id
stmt.close(nil)
print <<EOS
Registered query with id #{query_id}

EOS

# wait for events to come through
print <<EOS
In another session, modify the results of the query

#{sql_text}

Use Ctrl-C to terminate or wait for 100 seconds
EOS

20.times do
  puts "Waiting for notifications..."
  sleep(5)
end

# clean up
subscr.close
conn.close(nil, nil)

puts "Done."
