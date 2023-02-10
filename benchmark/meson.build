project('SC_BENCHMARK', 'C')

project_name = 'sc_benchmark'
project_name_abbreviation = 'sc'

deps = []
deps += dependency('libdpdk')

inc_dirs  = []
# benchmark 
inc_dirs += include_directories('include')

sources = [
	project_name_abbreviation + '_main.c',
    project_name_abbreviation + '_mbuf.c',
    project_name_abbreviation + '_port.c'
]

executable(project_name, sources,
	dependencies : deps,
	include_directories: inc_dirs,
	install: false)