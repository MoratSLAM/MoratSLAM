#!/usr/bin/env python3

import csv
import os

import rclpy
from rclpy.node import Node

from sensor_msgs.msg import NavSatFix
from topological_msgs.msg import TopologicalMap


class DataToCSV(Node):

    def __init__(self):
        super().__init__("data_to_csv")

        # =====================================================
        # Output directory
        # =====================================================

        self.output_dir = os.path.expanduser("/home/iarley_santos/Documents/tcc/topological_logs")
        os.makedirs(self.output_dir, exist_ok=True)

        self.nodes_file = os.path.join(
            self.output_dir,
            "topological_nodes.csv"
        )

        self.edges_file = os.path.join(
            self.output_dir,
            "topological_edges.csv"
        )

        self.gps_file = os.path.join(
            self.output_dir,
            "gps_log.csv"
        )

        # =====================================================
        # Create GPS CSV
        # =====================================================

        with open(self.gps_file, "w", newline="") as f:

            writer = csv.writer(f)

            writer.writerow([
                "timestamp",
                "latitude",
                "longitude",
                "altitude"
            ])

        # =====================================================
        # Create empty Nodes CSV
        # =====================================================

        with open(self.nodes_file, "w", newline="") as f:

            writer = csv.writer(f)

            writer.writerow([
                "node_id",
                "x",
                "y"
            ])

        # =====================================================
        # Create empty Edges CSV
        # =====================================================

        with open(self.edges_file, "w", newline="") as f:

            writer = csv.writer(f)

            writer.writerow([
                "source_id",
                "destination_id"
            ])

        # =====================================================
        # Subscribers
        # =====================================================

        self.map_sub = self.create_subscription(
            TopologicalMap,
            "/irat_red/ExperienceMap/Map",
            self.map_callback,
            10
        )

        self.gps_sub = self.create_subscription(
            NavSatFix,
            "/gps",
            self.gps_callback,
            10
        )

        self.get_logger().info("DataToCSV node started.")

    # ==========================================================
    # GPS CALLBACK
    # ==========================================================

    def gps_callback(self, msg: NavSatFix):

        timestamp = self.get_clock().now().nanoseconds / 1e9

        with open(self.gps_file, "a", newline="") as f:

            writer = csv.writer(f)

            writer.writerow([
                timestamp,
                msg.latitude,
                msg.longitude,
                msg.altitude
            ])

    # ==========================================================
    # TOPOLOGICAL MAP CALLBACK
    # ==========================================================

    def map_callback(self, msg: TopologicalMap):

        self.get_logger().info(
            f"Received map ({msg.node_count} nodes, {msg.edge_count} edges)"
        )

        # ------------------------------------------------------
        # Save Nodes
        # ------------------------------------------------------

        with open(self.nodes_file, "w", newline="") as f:

            writer = csv.writer(f)

            writer.writerow([
                "node_id",
                "x",
                "y"
            ])

            for node in msg.node:

                writer.writerow([
                    node.id,
                    node.pose.position.x,
                    node.pose.position.y
                ])

        # ------------------------------------------------------
        # Save Edges
        # ------------------------------------------------------

        with open(self.edges_file, "w", newline="") as f:

            writer = csv.writer(f)

            writer.writerow([
                "source_id",
                "destination_id"
            ])

            for edge in msg.edge:

                writer.writerow([
                    edge.source_id,
                    edge.destination_id
                ])

        self.get_logger().info(
            f"Saved {msg.node_count} nodes and {msg.edge_count} edges."
        )


# ==============================================================
# Main
# ==============================================================

def main(args=None):

    rclpy.init(args=args)

    node = DataToCSV()

    try:
        rclpy.spin(node)

    except KeyboardInterrupt:
        node.get_logger().info("Stopping DataToCSV...")

    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()