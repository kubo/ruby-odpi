module ODPI
  class Pool
    def initialize(*args)
      params = (args.last.is_a? Hash) ? args.pop : {}
      case args.length
      when 0
        username = params[:username]
        password = params[:password]
        database = params[:database]
        auth_mode = params[:auth_mode]
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
        username, password, database, auth_mode = ODPI.parse_connect_string(args[0])
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
