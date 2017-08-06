require 'mkmf'
require 'yaml'
require 'pp'

class FuncDef
  attr_reader :name
  attr_reader :args
  attr_reader :args_dcl
  attr_reader :cancel_cb

  def initialize(key, val)
    @name = key
    @args = val["args"].collect do |arg|
      ArgDef.new(arg)
    end
    breakable = (not val.has_key?("break")) || val["break"]
    @args_dcl = if @args[0].dcl == 'dpiConn *conn' || !breakable
                  ""
                else
                  'dpiConn *conn, '
                end + @args.collect {|arg| arg.dcl}.join(', ')
    @cancel_cb = if breakable
                   "(void (*)(void *))dpiConn_breakExecution, conn"
                 else
                   "NULL, NULL"
                 end
  end

  class ArgDef
    attr_reader :dcl
    attr_reader :name

    def initialize(arg)
      /(\w+)\s*$/ =~ arg
      @dcl = arg
      @name = $1
    end
  end
end

func_defs = []
YAML.load(open('rbdpi-func.yml')).each do |key, val|
  if val["args"]
    func_defs << FuncDef.new(key, val)
  end
end

open('rbdpi-func.h', 'w') do |f|
  f.print(<<EOS)
#ifndef ODPI_FUNC_H
#define ODPI_FUNC_H 1
/* This file was created by extconf.rb */

EOS
  func_defs.each do |func|
    f.print(<<EOS)
int #{func.name}_without_gvl(#{func.args_dcl});
EOS
  end
  f.print(<<EOS)
#endif
EOS
end

open('rbdpi-func.c', 'w') do |f|
  f.print(<<EOS)
/* This file was created by extconf.rb */
#include <ruby.h>
#include <ruby/thread.h>
#include <dpi.h>
#include "rbdpi-func.h"
EOS
  func_defs.each do |func|
    f.print(<<EOS)

/* #{func.name} */
typedef struct {
EOS
    func.args.each do |arg|
      f.print(<<EOS)
    #{arg.dcl};
EOS
    end
    f.print(<<EOS)
} #{func.name}_arg_t;

static void *#{func.name}_cb(void *data)
{
    #{func.name}_arg_t *arg = (#{func.name}_arg_t *)data;
    int rv = #{func.name}(#{func.args.collect {|arg| 'arg->' + arg.name}.join(', ')});
    return (void*)(size_t)rv;
}

int #{func.name}_without_gvl(#{func.args_dcl})
{
    #{func.name}_arg_t arg;
    void *rv;
EOS
    func.args.each do |arg|
      f.print(<<EOS)
    arg.#{arg.name} = #{arg.name};
EOS
    end
    f.print(<<EOS)
    rv = rb_thread_call_without_gvl(#{func.name}_cb, &arg, #{func.cancel_cb});
    return (int)(size_t)rv;
}
EOS
  end
end

$CFLAGS += " -I../../odpi/include"

$objs = (Dir['../../odpi/src/*.c'] + Dir['*.c']).collect do |src|
  File.basename(src, '.c') + '.o'
end

$VPATH << '../../odpi/src'

create_makefile('odpi_ext')
