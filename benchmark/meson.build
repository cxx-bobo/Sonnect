project('SC_BENCHMARK', 'C')

project_name = 'sc_benchmark'
project_name_abbreviation = 'sc'

# specified dependencies
dep = []
dep += dependency('libdpdk')
dep += meson.get_compiler('c').find_library('gmp')
dep += meson.get_compiler('c').find_library('pthread')

# specified which application to use
app = 'sketch'
add_global_arguments('-DAPP_' + app.to_upper(), language: 'c')

# specified the include path
inc_dirs = include_directories('include', 'include/' + project_name_abbreviation + '_' + app)

# scan all needed source files
glob = run_command('python3', files('./scan_src.py'), app)
sources = glob.stdout().strip().split('\n')
# message(sources)

# build executable
executable(project_name, sources,
	dependencies : dep,
	c_args: '-D_GNU_SOURCE',
	include_directories: inc_dirs
)