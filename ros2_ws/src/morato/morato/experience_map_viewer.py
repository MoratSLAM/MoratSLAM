#!/usr/bin/env python3

import matplotlib
matplotlib.use("TkAgg")

import matplotlib.pyplot as plt

import rclpy
from rclpy.node import Node

from topological_msgs.msg import TopologicalMap


class ExperienceMapViewer(Node):

    def __init__(self):
        super().__init__("experience_map_viewer")

        self.subscription = self.create_subscription(
            TopologicalMap,
            "/irat_red/ExperienceMap/Map",
            self.map_callback,
            10)

        self.latest_map = None

        plt.ion()

        self.fig, self.ax = plt.subplots(figsize=(8, 8))
        self.fig.canvas.manager.set_window_title("Topological Map")

        self.timer = self.create_timer(0.1, self.update_plot)

        self.get_logger().info("Experience Map Viewer started.")

    ####################################################################

    def map_callback(self, msg):
        self.latest_map = msg

    ####################################################################

    def update_plot(self):

        if self.latest_map is None:
            return

        msg = self.latest_map

        self.ax.clear()

        # --------------------------------------------------------------
        # Cria um dicionário id -> nó
        # --------------------------------------------------------------

        nodes = {}

        xs = []
        ys = []

        for n in msg.node:

            nodes[n.id] = n

            xs.append(n.pose.position.x)
            ys.append(n.pose.position.y)

        # --------------------------------------------------------------
        # Desenha as conexões
        # --------------------------------------------------------------

        for e in msg.edge:

            if e.source_id not in nodes:
                continue

            if e.destination_id not in nodes:
                continue

            p1 = nodes[e.source_id].pose.position
            p2 = nodes[e.destination_id].pose.position

            self.ax.plot(
                [p1.x, p2.x],
                [p1.y, p2.y],
                linewidth=1,
                color="black",
                zorder=1
            )

        # --------------------------------------------------------------
        # Desenha os nós
        # --------------------------------------------------------------

        self.ax.scatter(
            xs,
            ys,
            s=30,
            color="royalblue",
            zorder=2
        )

        self.ax.set_title(
            f"Topological Map | Nodes: {msg.node_count} | Edges: {msg.edge_count}"
        )

        self.ax.set_xlabel("X [m]")
        self.ax.set_ylabel("Y [m]")

        self.ax.grid(True)
        self.ax.set_aspect("equal", adjustable="box")

        self.fig.canvas.draw_idle()
        self.fig.canvas.flush_events()


########################################################################


def main(args=None):

    rclpy.init(args=args)

    node = ExperienceMapViewer()

    try:
        while rclpy.ok():

            rclpy.spin_once(node, timeout_sec=0.05)

            plt.pause(0.001)

    except KeyboardInterrupt:
        pass

    plt.close("all")

    node.destroy_node()
    rclpy.shutdown()


########################################################################

if __name__ == "__main__":
    main()