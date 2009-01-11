#!./waf-1.5.2


VERSION='0.1'
APPNAME='soliloque-server'
srcdir = '.'
blddir = 'output'
SOURCES='main_serv.c server.c channel.c player.c array.c connection_packet.c crc.c packet_tools.c acknowledge_packet.c control_packet.c strndup.c'

def set_options(opt):
  pass

def configure(conf):
  conf.check_tool('gcc')
  # default environment
  conf.setenv('default')
  conf.env['CFLAGS']='-O2'
  # Check for strndup (not present on OSX
  conf.check(cflags='-D_GNU_SOURCE', define_name='HAVE_STRNDUP', function_name='strndup', header_name='string.h')
  conf.write_config_header('config.h')

def build(bld):
  sol_serv = bld.new_task_gen()
  sol_serv.features = "cc cprogram"
  sol_serv.source = SOURCES
  sol_serv.target = APPNAME
  sol_serv.includes = '.'
  sol_serv.install_path = '${PREFIX}/bin'
  sol_serv.defines = ''
  sol_serv.cflags = '-O2 -Wall -D_GNU_SOURCE'
