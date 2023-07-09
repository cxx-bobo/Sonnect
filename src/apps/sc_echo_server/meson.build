# ========== basic macro ==========
# specified the execute role
# [1] client / server
role = 'server'
add_global_arguments('-DROLE_' + role.to_upper(), language: ['c', 'cpp'])

# ========== application specific macro ==========
sc_echo_server_get_latency = true
if sc_echo_server_get_latency
    add_global_arguments('-DSC_ECHO_SERVER_GET_LATENCY', language: ['c', 'cpp'])
endif
