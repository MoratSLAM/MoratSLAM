import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry

class GetLinearVelocity(Node):

    def __init__(self):
        super().__init__('get_linear_velocity')

        # Subscriber para /odom
        self.subscription = self.create_subscription(Odometry, '/odom', self.odom_callback, 10)

        # Arquivo onde os dados serão gravados
        self.output_file = 'velocity_linear.txt'
        self.get_logger().info(f"Salvando velocidades em: {self.output_file}")

    def odom_callback(self, msg):
        # Lê velocidade linear (vx, vy, vz)
        vx = msg.twist.twist.linear.x
        vy = msg.twist.twist.linear.y
        vz = msg.twist.twist.linear.z

        # Monta a string
        line = f"{vx:.5f}, {vy:.5f}, {vz:.5f}\n"

        # Salva no arquivo
        with open(self.output_file, 'a') as f:
            f.write(line)

        # Log simples
        self.get_logger().info(f"Velocidade linear: {line.strip()}")


def main(args=None):
    rclpy.init(args=args)
    node = GetLinearVelocity()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
