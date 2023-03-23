project('SOCONNECT', 'C',
	# Get version number from file.
	version: run_command(find_program('cat'),
		files('./VERSION'), check: true).stdout().strip(),
)

# specified which application to use
app = 'sha'
add_global_arguments('-DAPP_' + app.to_upper(), language: 'c')

# project name
project_name = app
project_name_abbreviation = 'sc'

# set configuration file for the application
app_conf_path = '"../conf/apps/' + app + '.conf"'
add_global_arguments('-DAPP_CONF_PATH=' + app_conf_path, language: 'c')

# set header file of the application
app_header_file_path = '"sc_' + app + '/' + app + '.h"'
add_global_arguments('-DAPP_HEADER_FILE_PATH=' + app_header_file_path, language: 'c')

# specified the sub directory to include more meson.build file
subdir('src/apps/' + project_name_abbreviation + '_' + app)

# set the SC_CLOSE_MOCK_MACRO to clear mock macro definition for developing
add_global_arguments('-DSC_CLOSE_MOCK_MACRO', language: 'c')

# indicator of whether need ARM/x86 SIMD
need_simd = false

# ==================== Third-party Library Detection ====================

# check whether doca exist
doca_path 				= '/opt/mellanox/doca'
doca_version			= ''
message('assume DOCA is located at ' + doca_path)
has_doca 				= false
doca_common_dep 		= dependency('doca-common', required: false)
doca_argp_dep 			= dependency('doca-argp', required: false)
doca_apsh_dep 			= dependency('doca-apsh', required: false)
doca_comm_channel_dep 	= dependency('doca-comm-channel', required: false)
doca_compress_dep 		= dependency('doca-compress', required: false)
doca_dma_dep 			= dependency('doca-dma', required: false)
doca_dpi_dep 			= dependency('doca-dpi', required: false)
doca_flow_dep 			= dependency('doca-flow', required: false)
doca_regex_dep 			= dependency('doca-regex', required: false)
doca_sha_dep 			= dependency('doca-sha', required: false)
doca_telemetry_dep 		= dependency('doca-telemetry', required: false)

# DOCA hardware resources disable flag
doca_enable_sha = true

if doca_common_dep.found() and doca_argp_dep.found() and doca_apsh_dep.found() \
		and doca_comm_channel_dep.found() and doca_compress_dep.found() \
		and doca_dma_dep.found() and doca_dpi_dep.found() \
		and doca_flow_dep.found() and doca_regex_dep.found() \
		and doca_sha_dep.found() and doca_telemetry_dep.found()
	# set doca as founded
	has_doca = true
	add_global_arguments('-DSC_HAS_DOCA', language: 'c')

	# set doca version
	doca_version = run_command(
		find_program('cat'), files(doca_path + '/applications/VERSION'), check: true).stdout().strip()
	message('found DOCA ' + doca_version)
	add_global_arguments('-DSC_DOCA_MAIN_VERSION=' + doca_version[0], language: 'c')
	add_global_arguments('-DSC_DOCA_SUB_VERSION=' + doca_version[2], language: 'c')

	# specified necessary doca resources according to application
	if doca_enable_sha == true		
		if app == 'sketch' or app == 'sha'
			add_global_arguments('-DSC_NEED_DOCA_SHA', language: 'c')
		endif
		message('enabled DOCA SHA engine')
	else
		need_simd = true
	endif
endif

# check whether cuda exist
cuda_path 	= '/usr/local/cuda'
message('assume CUDA is located at ' + cuda_path)
has_cuda 	= false
cuda_dep 	= dependency('cuda', version : '>=11', required: false)
if cuda_dep.found()
	has_cuda = true
	add_global_arguments('-DSC_HAS_CUDA', language: 'c')
endif

# =======================================================================


# ==================== Specified All Dependent Library ==================

# basic dependencies
dep = []
dep += dependency('libdpdk', required: true)
dep += meson.get_compiler('c').find_library('pthread')
dep += meson.get_compiler('c').find_library('gmp')

# DOCA dependencies (optional)
if has_doca
	dep += doca_common_dep
	dep += doca_argp_dep
	dep += doca_apsh_dep
	dep += doca_comm_channel_dep
	dep += doca_compress_dep
	dep += doca_dma_dep
	dep += doca_dpi_dep
	dep += doca_flow_dep
	dep += doca_regex_dep
	dep += doca_sha_dep
	dep += doca_telemetry_dep
endif

# CUDA dependencies (optional)
if has_cuda
	dep += cuda_dep
endif

# =======================================================================


# ==
# simd = import('unstable-simd')
# sc_simd_lib = []
# if need_simd
# 	simd_variants = [
# 		{ 'mmx' : ['remap_mmx.c', 'svolume_mmx.c'] },
# 		{ 'sse' : ['remap_sse.c', 'sconv_sse.c', 'svolume_sse.c'] },
# 		{ 'neon' : ['remap_neon.c', 'sconv_neon.c', 'mix_neon.c'] },
# 	]

# 	foreach simd_kwargs : simd_variants
# 		# ARM NEON support
# 		if host_machine.cpu_family() == 'arm' and 'neon' in simd_kwargs
# 			if not meson.get_compiler('c').compiles('''
# 				#include <arm_neon.h>
# 					int main() {
# 						return sizeof(uint8x8_t) + sizeof(int32x4_t) + sizeof(float32x4_t);
# 					}
# 				''', name : 'neon code'
# 			)
# 				continue
# 			endif
# 		endif

# 		sc_simd = simd.check('sc_simd',
# 			kwargs : simd_kwargs,
# 			implicit_include_directories : false,
# 			compiler: meson.get_compiler('c'),
# 		)

# 		sc_simd_lib += sc_simd[0]
# 		# cdata.merge_from(libpulsecore_simd[1])
# 	endforeach
# endif

# ==================== Specified All Dependent Headers ==================

# scan all needed header files
glob = run_command('python3', files('./scan_include.py'), project_name_abbreviation, app, has_doca.to_string(), doca_path, doca_version, has_cuda.to_string(), cuda_path)
inc_dirs = include_directories(glob.stdout().strip().split('\n'))
# message(glob.stdout().strip().split('\n'))

# =======================================================================


# ====================== Specifed All Source Files ======================

# scan all needed source files
glob = run_command('python3', files('./scan_src.py'), app, has_doca.to_string(), doca_path, doca_version, has_cuda.to_string(), cuda_path)
sources = glob.stdout().strip().split('\n')
# message(sources)

# =======================================================================


# ========================= Specifed All CFLAGS =========================

## Note: we detect different DPDK, DOCA and CUDA version inside meson and source files,
##		so we block all deprecated-declarations warnings here to make log output of 
##		the build process cleaner
sc_c_args = ['-D_GNU_SOURCE', '-Wno-deprecated-declarations', '-g']

if need_simd
	if host_machine.cpu_family() == 'arm' or host_machine.cpu_family() == 'aarch64'
		# ref: https://github.com/noloader/SHA-Intrinsics#arm-sha
		sc_c_args += ['-march=armv8-a+crc+crypto']
	elif host_machine.cpu_family() == 'x86' or host_machine.cpu_family() == 'x86_64'
		# ref: https://github.com/noloader/SHA-Intrinsics#intel-sha
		sc_c_args += ['-msse4 -msha']
	endif
endif 
# =======================================================================


# build executable
executable(project_name, sources,
	dependencies : dep,
	c_args: sc_c_args,
	include_directories: inc_dirs,
	install: false
)