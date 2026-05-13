#!/bin/bash
set -e

# Source ROS 2
source /opt/ros/jazzy/setup.bash

# Source workspace se existir
if [ -f /ros2_ws/install/setup.bash ]; then
    source /ros2_ws/install/setup.bash
fi

echo "✅ ROS 2 Jazzy — Nalu ready"
echo "   ROS_DOMAIN_ID: $ROS_DOMAIN_ID"
echo "   RMW: $RMW_IMPLEMENTATION"

exec "$@"
