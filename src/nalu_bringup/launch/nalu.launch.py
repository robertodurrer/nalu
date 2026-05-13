"""
nalu.launch.py — Launch principal do Robô Nalu
Inicia todos os nós necessários para operação.
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():

    # Argumentos
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    log_level     = LaunchConfiguration('log_level', default='info')

    config_file = PathJoinSubstitution([
        FindPackageShare('nalu_bringup'),
        'config',
        'nalu_params.yaml'
    ])

    return LaunchDescription([

        DeclareLaunchArgument('use_sim_time', default_value='false',
                              description='Usar tempo de simulação'),
        DeclareLaunchArgument('log_level', default_value='info',
                              description='Nível de log'),

        LogInfo(msg='🤖 Iniciando Robô Nalu...'),

        # Nó base — interface com hardware
        Node(
            package='nalu_base',
            executable='nalu_base_node',
            name='nalu_base',
            namespace='nalu',
            output='screen',
            parameters=[config_file, {'use_sim_time': use_sim_time}],
            arguments=['--ros-args', '--log-level', log_level],
        ),
    ])
