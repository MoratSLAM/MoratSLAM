#!/usr/bin/env python3
import rclpy
from rclpy.node import Node

from nav_msgs.msg import Odometry
from sensor_msgs.msg import NavSatFix

import time
import csv
import os

class DataToCsv(Node):

    def __init__(self):
        super().__init__('data_to_csv')

        # ---------------- ODOM SUB ----------------
        self.subscription_odom = self.create_subscription(Odometry, '/irat_red/odom', self.odom_callback, 10)

        # ---------------- GPS SUB ----------------
        self.subscription_gps = self.create_subscription(NavSatFix, '/gps', self.gps_callback, 10)

        self.start_time = time.time()

        timestamp = time.strftime("%Y%m%d_%H%M%S")

        # ---------------- ODOM CSV ----------------
        self.odom_csv_filename = f"odom_velocity_{timestamp}.csv"
        self.odom_csv_file = open(self.odom_csv_filename, mode='w', newline='')
        self.odom_csv_writer = csv.writer(self.odom_csv_file)

        self.odom_csv_writer.writerow([
            "time",
            "lin_x", "lin_y", "lin_z",
            "ang_x", "ang_y", "ang_z"
        ])

        # ---------------- GPS CSV ----------------
        self.gps_csv_filename = f"gps_latlon_{timestamp}.csv"
        self.gps_csv_file = open(self.gps_csv_filename, mode='w', newline='')
        self.gps_csv_writer = csv.writer(self.gps_csv_file)

        self.gps_csv_writer.writerow([
            "time",
            "latitude",
            "longitude",
            "altitude"
        ])

        self.get_logger().info(
            f"Odom CSV: {os.path.abspath(self.odom_csv_filename)}"
        )
        self.get_logger().info(
            f"GPS CSV:  {os.path.abspath(self.gps_csv_filename)}"
        )

    # ---------------- ODOM CALLBACK ----------------
    def odom_callback(self, msg: Odometry):
        t = time.time() - self.start_time

        self.odom_csv_writer.writerow([
            t,
            msg.twist.twist.linear.x,
            msg.twist.twist.linear.y,
            msg.twist.twist.linear.z,
            msg.twist.twist.angular.x,
            msg.twist.twist.angular.y,
            msg.twist.twist.angular.z
        ])

    # ---------------- GPS CALLBACK ----------------
    def gps_callback(self, msg: NavSatFix):
        t = time.time() - self.start_time

        # Opcional: só grava se o GPS for válido
        if msg.status.status < 0:
            return

        self.gps_csv_writer.writerow([
            t,
            msg.latitude,
            msg.longitude,
            msg.altitude
        ])

    def destroy_node(self):
        self.odom_csv_file.close()
        self.gps_csv_file.close()
        self.get_logger().info("Arquivos CSV fechados com sucesso.")
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)

    node = DataToCsv()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()