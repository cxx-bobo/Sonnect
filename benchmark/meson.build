project('SC_BENCHMARK', 'C')

project_name = 'sc_benchmark'
project_name_abbreviation = 'sc'

dpdk = dependency('libdpdk')
inc_dirs = include_directories('include')

source_folder = './src/'

sources = files(
	source_folder + project_name_abbreviation + '_main.c',
    source_folder + project_name_abbreviation + '_mbuf.c',
    source_folder + project_name_abbreviation + '_port.c',
	source_folder + project_name_abbreviation + '_utils.c'
)

executable(project_name, sources,
	dependencies : dpdk,
	include_directories: inc_dirs
)