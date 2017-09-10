#-----------------------------------------------------------------------------
# test_bind_objects.rb
#   Tests simple binding of objects.
#-----------------------------------------------------------------------------
#
# This was ported from TestBindObjects.c in ODPI-C.
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
conn = ODPI::connect($main_user, $main_password, $connect_string)

obj = UdtObject.new(conn)
obj.numbervalue = 13
obj.stringvalue = 'Test String'

stmt = conn.prepare('begin :1 := pkg_TestBindObject.GetStringRep(:2); end;')
stmt.bind(1, nil, String, :length => 100)
stmt.bind(2, obj)
stmt.execute
puts "String rep: '#{stmt[1]}'"
stmt.close
conn.close

puts "Done."
