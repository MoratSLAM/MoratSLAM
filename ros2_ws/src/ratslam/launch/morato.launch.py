import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription, LaunchContext
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def prepare_nodes(context: LaunchContext, *args, **kwargs):
    # 1. Fetch the argument from the terminal and perform the calculation
    compass_angle_str = LaunchConfiguration('compass_angle').perform(context)
    compass_angle = float(compass_angle_str)
    ratslam_angle = (90.0 - compass_angle) % 360.0
    
    print(f"\n[RatSLAM] Compass read: {compass_angle}° | Injecting into EM: {ratslam_angle}°\n")

    # 2. Prepare paths (equivalent to $(find-pkg-share) in XML)
    ratslam_share = get_package_share_directory('ratslam')
    config_yaml = os.path.join(ratslam_share, 'config', 'config_morato.yaml')
    media_dir = os.path.join(ratslam_share, 'media')

    # 3. Common parameters for all nodes
    common_params = {
        'use_sim_time': False,
        'topic_root': 'irat_red',
        'media_path': media_dir,
        'image_file': 'morato.tga'
    }

    # 4. Node 1: Local View (ratslam_lv)
    node_lv = Node(
        package='ratslam',
        executable='ratslam_lv',
        name='ratslam_view_template',
        output='screen',
        remappings=[('_image_transport', 'compressed')],
        parameters=[config_yaml, common_params]
    )

    # 5. Node 2: Pose Cells (ratslam_pc)
    node_pc = Node(
        package='ratslam',
        executable='ratslam_pc',
        name='ratslam_pose_cells',
        output='screen',
        remappings=[('_image_transport', 'compressed')],
        parameters=[config_yaml, common_params]
    )

    # 6. Node 3: Experience Map (ratslam_em)
    # Copy common parameters and add the calculated angle
    params_em = common_params.copy()
    params_em['exp_initial_em_deg'] = ratslam_angle

    node_em = Node(
        package='ratslam',
        executable='ratslam_em',
        name='ratslam_experience_map',
        output='screen',
        remappings=[('_image_transport', 'compressed')],
        parameters=[config_yaml, params_em]
    )

    return [node_lv, node_pc, node_em]

def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'compass_angle',
            default_value='0.0',
            description='Initial rover compass angle in degrees (0=North)'
        ),
        OpaqueFunction(function=prepare_nodes)
    ])