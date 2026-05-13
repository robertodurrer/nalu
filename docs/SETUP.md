# Setup do Ambiente de Desenvolvimento — Nalu

## 1. Clonar o repositório

```bash
git clone https://github.com/robertodurrer/nalu.git
cd nalu
```

## 2. Subir com Docker

```bash
# Build da imagem
docker compose build

# Iniciar o robô
docker compose up

# Shell de desenvolvimento
docker compose --profile dev run nalu_dev
```

## 3. Desenvolvimento sem Docker (ROS 2 Jazzy local)

```bash
source /opt/ros/jazzy/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
ros2 launch nalu_bringup nalu.launch.py
```

## 4. USB Passthrough no Proxmox

Para passar o hardware serial para a VM:

```bash
# No host Proxmox, identificar o dispositivo
lsusb

# Configurar passthrough (substituir 100 pelo ID da VM)
qm set 100 --usb0 host=<vendorid>:<productid>
```

## 5. Verificar tópicos

```bash
ros2 topic list
ros2 topic echo /nalu/status
ros2 topic echo /nalu/odom
```

## 6. Enviar comandos de teste

```bash
ros2 topic pub /nalu/cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.2}, angular: {z: 0.0}}"
```
