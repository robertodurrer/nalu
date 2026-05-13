# nalu_msgs/msg/Status.msg
# Status geral do Robô Nalu

std_msgs/Header header

bool  is_connected        # Conectado ao hardware
bool  is_emergency_stop   # E-stop ativo
float32 battery_voltage   # Tensão da bateria (V)
float32 battery_percent   # Percentual da bateria (0.0 - 1.0)
string state              # Estado atual: "idle", "running", "error"
string error_msg          # Mensagem de erro (se houver)
