# ========== basic macro ==========
# specified the execute role
# [1] client / server
role = 'server'
add_global_arguments('-DROLE_' + role.to_upper(), language: 'c')

# ========== application specific macro ==========
# specified the execute mode
# [1] throughput / [2] latency
mode = 'throughput'
add_global_arguments('-DMODE_' + mode.to_upper(), language: 'c')