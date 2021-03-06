# pool.rb -- part of ruby-odpi
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
  class Pool
    def initialize(*args)
      params = (args.last.is_a? Hash) ? args.pop : {}
      case args.length
      when 0
        username = params[:username]
        password = params[:password]
        database = params[:database]
      when 1
        username, password, database = ODPI.parse_connect_string(args[0])
      else
        username, password, database = args
      end
      @pool = Dpi::Pool.new(username, password, database, params)
    end

    def connection(*args)
      params = (args.last.is_a? Hash) ? args.pop : {}
      case args.length
      when 0
        username = params[:username]
        password = params[:password]
        auth_mode = params[:auth_mode]
      when 1
        username, password, _, auth_mode = ODPI.parse_connect_string(args[0])
      else
        username, password, auth_mode = args
      end
      Connection.new(@pool.connection(username, password, auth_mode, params), false)
    end

    def close
      @pool.close(nil)
    end

  end # pool
end
