FROM osrf/ros:jazzy-ros-base

# Metadados
LABEL maintainer="robertodurrer"
LABEL project="nalu"
LABEL version="0.1.0"

# Evitar interação durante apt
ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DOMAIN_ID=0
ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

# Dependências do sistema
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-pip \
    ros-jazzy-rmw-cyclonedds-cpp \
    ros-jazzy-diagnostic-updater \
    ros-jazzy-diagnostic-msgs \
    ros-jazzy-sensor-msgs \
    ros-jazzy-geometry-msgs \
    ros-jazzy-nav-msgs \
    ros-jazzy-std-msgs \
    ros-jazzy-tf2 \
    ros-jazzy-tf2-ros \
    udev \
    usbutils \
    && rm -rf /var/lib/apt/lists/*

# Criar workspace
WORKDIR /ros2_ws

# Copiar pacotes fonte
COPY src/ src/

# Build do workspace
RUN /bin/bash -c "source /opt/ros/jazzy/setup.bash && \
    rosdep update && \
    rosdep install --from-paths src --ignore-src -r -y && \
    colcon build --symlink-install \
    --cmake-args -DCMAKE_BUILD_TYPE=Release"

# Copiar entrypoint
COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]
