module ODPI

  def self.connect(*args)
    params = (args.last.is_a? Hash) ? args.pop : {}
    case args.length
    when 0
      username = params[:username]
      password = params[:password]
      database = params[:database]
      auth_mode = params[:auth_mode]
    when 1
      username, password, database, auth_mode = ODPI.parse_connect_string(args[0])
    else
      username, password, database, auth_mode = args
    end
    Connection.new(Dpi::Conn.new(username, password, database, auth_mode, params), true)
  end

  class Connection
    def initialize(conn, is_standalone)
      @conn = conn
      @is_standalone = is_standalone
    end

    def close
      @conn.close(nil, nil)
    end

    def new_subscription(params)
      @conn.new_subscription(params)
    end
  end # Connection
end
