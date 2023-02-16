# ========== basic macro ==========
# specified the execute role
# [1] client / server
role = 'server'
add_global_arguments('-DROLE_' + role.to_upper(), language: 'c')

# ========== application specific macro ==========
# specified the execute mode
# [1] throughput / accuracy / latency
mode = 'latency'
add_global_arguments('-DMODE_' + mode.to_upper(), language: 'c')