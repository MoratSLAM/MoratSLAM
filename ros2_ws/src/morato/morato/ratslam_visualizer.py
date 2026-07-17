import math

import rclpy
from rclpy.node import Node

# Mensagem do mapa customizado
from topological_msgs.msg import TopologicalMap

# Mensagens de visualização e geometria padrão do ROS2
from visualization_msgs.msg import Marker, MarkerArray
from std_msgs.msg import ColorRGBA
from geometry_msgs.msg import Point, PoseStamped
from sensor_msgs.msg import NavSatFix

# Raio médio da Terra (WGS84), usado na projeção equiretangular local
EARTH_RADIUS = 6378137.0


class RatSlamVisualizer(Node):
    def __init__(self):
        super().__init__('ratslam_visualizer')

        # --- SUBSCRIBERS ---
        # 1. Tópico do Mapa
        self.map_sub = self.create_subscription(
            TopologicalMap,
            '/irat_red/ExperienceMap/Map',
            self.map_callback,
            10
        )

        # 2. Tópico da Posição do Robô (RobotPose)
        self.pose_sub = self.create_subscription(
            PoseStamped,
            '/irat_red/ExperienceMap/RobotPose',
            self.pose_callback,
            10
        )

        # 3. Tópico do GPS (NavSatFix)
        self.gps_sub = self.create_subscription(
            NavSatFix,
            '/gps',  # <-- ajuste aqui para o nome real do seu tópico de GPS
            self.gps_callback,
            10
        )

        # --- PUBLISHERS ---
        # 1. Publicador do Mapa (Nós e Arestas)
        self.map_pub = self.create_publisher(MarkerArray, '/ratslam/map_markers', 10)

        # 2. Publicador do Robô
        self.robot_pub = self.create_publisher(Marker, '/ratslam/robot_marker', 10)

        # 3. Publicador do rastro de GPS
        self.gps_pub = self.create_publisher(Marker, '/ratslam/gps_marker', 10)

        # Estado interno para acumular o rastro do GPS
        self.gps_origin = None      # (lat0, lon0) usado como referência local
        self.gps_points = []        # lista de geometry_msgs/Point já convertidos

        self.get_logger().info('Visualizador iniciado! Lendo Map, RobotPose e GPS...')

    def map_callback(self, msg):
        if msg.node_count == 0:
            return

        marker_array = MarkerArray()
        frame = msg.header.frame_id if msg.header.frame_id else 'map'

        # --- 1. MARCADOR DOS NÓS (Esferas Ciano) ---
        nodes_marker = Marker()
        nodes_marker.header.frame_id = frame
        nodes_marker.header.stamp = self.get_clock().now().to_msg()
        nodes_marker.ns = 'ratslam_nodes'
        nodes_marker.id = 0
        nodes_marker.type = Marker.SPHERE_LIST
        nodes_marker.action = Marker.ADD
        nodes_marker.scale.x = 0.1
        nodes_marker.scale.y = 0.1
        nodes_marker.scale.z = 0.1
        nodes_marker.color = ColorRGBA(r=0.0, g=1.0, b=1.0, a=1.0)

        node_positions = {}
        for node in msg.node:
            nodes_marker.points.append(node.pose.position)
            node_positions[node.id] = node.pose.position
        marker_array.markers.append(nodes_marker)

        # --- 2. MARCADOR DAS ARESTAS (Linhas Amarelas) ---
        if msg.edge_count > 0:
            edges_marker = Marker()
            edges_marker.header.frame_id = frame
            edges_marker.header.stamp = self.get_clock().now().to_msg()
            edges_marker.ns = 'ratslam_edges'
            edges_marker.id = 1
            edges_marker.type = Marker.LINE_LIST
            edges_marker.action = Marker.ADD
            edges_marker.scale.x = 0.03
            edges_marker.color = ColorRGBA(r=1.0, g=1.0, b=0.0, a=0.8)

            for edge in msg.edge:
                if edge.source_id in node_positions and edge.destination_id in node_positions:
                    edges_marker.points.append(node_positions[edge.source_id])
                    edges_marker.points.append(node_positions[edge.destination_id])
            marker_array.markers.append(edges_marker)

        self.map_pub.publish(marker_array)

    def pose_callback(self, msg):
        robot_marker = Marker()

        # 1. FORÇAR O FRAME_ID E O TEMPO
        # Ignoramos o header da mensagem original e cravamos no 'map'
        robot_marker.header.frame_id = 'map'
        robot_marker.header.stamp = self.get_clock().now().to_msg()

        robot_marker.ns = 'ratslam_robot'
        robot_marker.id = 999

        robot_marker.type = Marker.SPHERE
        robot_marker.action = Marker.ADD

        # Tamanho gigante que você configurou
        robot_marker.scale.x = 1.5
        robot_marker.scale.y = 0.7
        robot_marker.scale.z = 0.1

        robot_marker.color = ColorRGBA(r=1.0, g=0.0, b=0.0, a=1.0)

        robot_marker.pose = msg.pose

        # 2. VALIDAR O QUATERNION (A Prova de Balas)
        # Se a orientação vier zerada, forçamos um quaternion válido (sem rotação)
        if (robot_marker.pose.orientation.x == 0.0 and
                robot_marker.pose.orientation.y == 0.0 and
                robot_marker.pose.orientation.z == 0.0 and
                robot_marker.pose.orientation.w == 0.0):

            robot_marker.pose.orientation.w = 1.0

        self.robot_pub.publish(robot_marker)

    def gps_callback(self, msg: NavSatFix):
        # Ignora leituras sem fix válido
        if msg.status.status < 0:
            return

        # Na primeira leitura válida, define a origem local (lat0, lon0)
        if self.gps_origin is None:
            self.gps_origin = (msg.latitude, msg.longitude)
            self.get_logger().info(
                f'Origem do GPS definida em lat={msg.latitude:.7f}, lon={msg.longitude:.7f}'
            )

        x, y = self.latlon_to_local_xy(msg.latitude, msg.longitude)

        point = Point()
        point.x = x
        point.y = y
        point.z = 0.0
        self.gps_points.append(point)

        self.publish_gps_marker()

    def latlon_to_local_xy(self, lat, lon):
        """Converte lat/lon para coordenadas locais (x=Leste, y=Norte) em metros,
        usando uma projeção equiretangular simples referenciada à primeira posição
        recebida. Suficiente para trajetórias curtas/médias; para áreas muito
        grandes, considere usar UTM."""
        lat0, lon0 = self.gps_origin

        lat_rad = math.radians(lat)
        lon_rad = math.radians(lon)
        lat0_rad = math.radians(lat0)
        lon0_rad = math.radians(lon0)

        x = (lon_rad - lon0_rad) * math.cos(lat0_rad) * EARTH_RADIUS
        y = (lat_rad - lat0_rad) * EARTH_RADIUS

        return x, y

    def publish_gps_marker(self):
        gps_marker = Marker()
        gps_marker.header.frame_id = 'map'
        gps_marker.header.stamp = self.get_clock().now().to_msg()
        gps_marker.ns = 'ratslam_gps'
        gps_marker.id = 2
        gps_marker.type = Marker.LINE_STRIP
        gps_marker.action = Marker.ADD
        gps_marker.scale.x = 0.08
        gps_marker.color = ColorRGBA(r=0.0, g=1.0, b=0.0, a=1.0)
        gps_marker.pose.orientation.w = 1.0
        gps_marker.points = self.gps_points

        self.gps_pub.publish(gps_marker)


def main(args=None):
    rclpy.init(args=args)
    node = RatSlamVisualizer()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()