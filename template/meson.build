project('SC_BENCHMARK', 'C')

project_name = 'sc_benchmark'
project_name_abbreviation = 'sc'

dep = []
dep += dependency('libdpdk')
dep += meson.get_compiler('c').find_library('gmp')

inc_dirs = include_directories('include')

source_folder = './src/'

sources = files(
	source_folder + project_name_abbreviation + '_main.c',
    source_folder + project_name_abbreviation + '_mbuf.c',
    source_folder + project_name_abbreviation + '_port.c',
	source_folder + project_name_abbreviation + '_utils.c',
	source_folder + project_name_abbreviation + '_worker.c'
)

executable(project_name, sources,
	dependencies : dep,
	include_directories: inc_dirs
)