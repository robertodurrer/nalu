# 🤖 Nalu — ROS 2 Robot

> Robot Operating System 2 (Jazzy Jalisco) — Hardware Real

---

## 📦 Pacotes

| Pacote | Descrição |
|---|---|
| `nalu_bringup` | Launch files principais e configurações |
| `nalu_base` | Nó de controle e interface com hardware |
| `nalu_description` | URDF e descrição física do robô |
| `nalu_msgs` | Mensagens e serviços customizados |

---

## 🚀 Início Rápido

### Pré-requisitos
- Docker Engine
- Docker Compose

### Clonar e iniciar

```bash
git clone https://github.com/robertodurrer/nalu.git
cd nalu

# Build da imagem
docker compose build

# Iniciar o robô
docker compose up
```

---

## 🏗️ Estrutura do Projeto

```
nalu/
├── src/
│   ├── nalu_bringup/        # Launch files e config
│   ├── nalu_base/           # Controle e hardware
│   ├── nalu_description/    # URDF e meshes
│   └── nalu_msgs/           # Msgs e srvs customizados
├── docker/
│   ├── Dockerfile
│   └── entrypoint.sh
├── docker-compose.yml
├── .github/
│   └── workflows/
│       └── ci.yml
└── docs/
```

---

## 🐳 Docker

```bash
# Build
docker compose build

# Executar nós
docker compose up nalu_base

# Shell interativo
docker compose run --rm nalu bash
```

---

## 🔧 Desenvolvimento Local (sem Docker)

```bash
# Dentro do container ou com ROS 2 Jazzy instalado
cd nalu
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install
source install/setup.bash

# Iniciar robô
ros2 launch nalu_bringup nalu.launch.py
```

---

## 📡 Tópicos Principais

| Tópico | Tipo | Descrição |
|---|---|---|
| `/nalu/cmd_vel` | `geometry_msgs/Twist` | Comandos de velocidade |
| `/nalu/odom` | `nav_msgs/Odometry` | Odometria |
| `/nalu/status` | `nalu_msgs/Status` | Status do robô |
| `/nalu/battery` | `sensor_msgs/BatteryState` | Estado da bateria |

---

## 📝 Licença

Apache 2.0 — veja [LICENSE](LICENSE)
