# odpi.rb -- part of ruby-odpi
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

require 'odpi_ext.so'
require 'odpi/bindtype.rb'
require 'odpi/connection.rb'
require 'odpi/object.rb'
require 'odpi/pool.rb'
require 'odpi/statement.rb'
require 'odpi/version.rb'

module ODPI

  # [Japanese] not registered in IANA character sets.
  RUBY_TO_ORACLE_ENCODING['eucJP-ms'] = 'JA16EUCTILDE'

  # [Japanese] registered in IANA character sets but ODPI-C doesn't know it.
  RUBY_TO_ORACLE_ENCODING['CP51932'] = 'JA16EUCTILDE'

  # [Japanese] ODPI-C incorrectly maps it to 'JA16SJIS'.
  RUBY_TO_ORACLE_ENCODING['Windows-31J'] = 'JA16SJISTILDE'

  # @private
  def self.parse_connect_string(conn_str)
    if /^([^(\s|\@)]*)\/([^(\s|\@)]*)(?:\@(\S+))?(?:\s+as\s+(\S*)\s*)?$/i =~ conn_str
      username = $1
      password = $2
      database = $3
      privilege = $4
      username = password = nil if username.length == 0 && password.length == 0
      privilege = privilege.downcase.to_sym if privilege
      [username, password, database, privilege]
    else
      raise ArgumentError, %Q{invalid connect string "#{conn_str}" (expect "username/password[@(tns_name|//host[:port]/service_name)][ as (sysdba|sysoper)]")}
    end
  end
end
