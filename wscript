#!/usr/bin/env python

import os
import sys
import Configure

VERSION='undefined'
APPNAME='soliloque-server'
srcdir = '.'
blddir = 'output'
SOURCES='main_serv.c server.c channel.c player.c array.c connection_packet.c crc.c packet_tools.c acknowledge_packet.c control_packet.c toolbox.c audio_packet.c ban.c server_stat.c configuration.c database.c registration.c server_privileges.c player_stat.c log.c queue.c packet_sender.c player_channel_privilege.c'
flags_dbg1= ['-Wall', '-Werror']
flags_dbg2= ['-Wno-unused-parameter', '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wpointer-arith']
flags_dbg2.extend(flags_dbg1)
flags_dbg3= ['-Wreturn-type', '-Wcast-qual', '-Wswitch', '-Wshadow', '-Wcast-align']
flags_dbg3.extend(flags_dbg2)


def set_options(opt):
  opt.add_option('--with-openssl', type='string', help='Define the location of openssl libraries.', dest='openssl')

def get_git_version():
  S = __import__('subprocess')
  """ try grab the current version number from git"""
  version = None
  if os.path.exists(".git"):
    try:
      version = S.Popen(['git', 'describe'], stdout=S.PIPE).communicate()[0].strip()
    except Exception, e:
      print e
  return version

def read_git_version():
  """Read version from git repo, or from GIT_VERSION"""
  version = get_git_version()
  if not version and os.path.exists("GIT_VERSION"):
    f = open("GIT_VERSION", "r")
    version = f.read().strip()
    f.close()
  if version:
    global VERSION
    VERSION = version

def dist():
  """Make the dist tarball and print its sha1sum"""
  def write_git_version():
    """write the revision to a file called GIT_VERSION, to grab
    the current version number from git when generating the dist
    tarball."""
    version = get_git_version()
    if not version:
      return False
    version_file = open("GIT_VERSION", "w")
    version_file.write(version + "\n")
    version_file.close()
    return True
  import Scripting
  read_git_version()
  write_git_version()
  filename = Scripting.dist(APPNAME, VERSION)
  os.spawnlp(os.P_WAIT, "sha1sum", "sha1sum", filename)

def configure(conf):
  import Options
  read_git_version()
  conf.check_tool('gcc')
  # Compile flags
  cflags = ['-O0', '-g', '-ggdb']
  cflags.extend(flags_dbg1)
  conf.env.append_unique('CCFLAGS', cflags)
  # default environment
  conf.setenv('default')
  conf.check_cfg(atleast_pkgconfig_version='0.0.0')
  conf.check_cfg(package='libconfig', args='--cflags --libs', uselib_store='LIBCONFIG', mandatory=True)
  conf.check_cc(lib='dbi', uselib_store='LIBDBI', mandatory=True)
  conf.check_cc(lib='pthread', uselib_store='PTHREAD', mandatory=True)
  # Check for OpenSSL library and support for SHA256
  if (Options.options.openssl):
    conf.check_cc(lib='crypto', cppflags='-I'+Options.options.openssl+'/include',
		    ldflags='-L'+Options.options.openssl+'/lib', uselib_store='OPENSSL', mandatory=True)
  else:
    conf.check_cc(lib='crypto', uselib_store='OPENSSL', mandatory=True)
  conf.check(define_name='HAVE_SHA256', uselib='OPENSSL', function_name='SHA256', header_name='openssl/sha.h', mandatory=True)
  # Check for strndup (not present on OSX)
  conf.check(cflags='-D_GNU_SOURCE', define_name='HAVE_STRNDUP', function_name='strndup', header_name='string.h', errmsg='internal')
  conf.define('VERSION', VERSION)
  conf.write_config_header('config.h')

def build(bld):
  sol_serv = bld.new_task_gen()
  sol_serv.features = "cc cprogram"
  sol_serv.source = SOURCES
  sol_serv.target = APPNAME
  sol_serv.includes = '.'
  sol_serv.install_path = '${PREFIX}/bin'
  sol_serv.defines = ['_GNU_SOURCE', '_BSD_SOURCE']
  sol_serv.uselib = 'LIBCONFIG PTHREAD LIBDBI OPENSSL'
