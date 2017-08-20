require 'odpi_ext.so'
require 'odpi/connection.rb'
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
