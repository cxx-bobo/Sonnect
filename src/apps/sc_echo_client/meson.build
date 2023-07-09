# ========== basic macro ==========
# specified the execute role
# [1] client / server
role = 'client'
add_global_arguments('-DROLE_' + role.to_upper(), language: ['c', 'cpp'])

# ========== application specific macro ==========
sc_echo_client_get_latency = false
if sc_echo_client_get_latency
    add_global_arguments('-DSC_ECHO_CLIENT_GET_LATENCY', language: ['c', 'cpp'])
endif
