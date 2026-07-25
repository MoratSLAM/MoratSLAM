import sys
import cv2
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CompressedImage
from rclpy.clock import Clock, ClockType

class GetImages(Node):
    def __init__(self):
        super().__init__("get_images")
        self.pub = self.create_publisher(CompressedImage, "/morato/camera/image/compressed", 10)
        self.cap = cv2.VideoCapture(0)

        if not self.cap.isOpened():
            self.get_logger().error("FATAL ERROR: Failed to OPEN the camera! Check connection or permissions.")
            sys.exit(1)
            
        self.get_logger().info("Camera opened successfully! Starting image capture...")

        self.timer = self.create_timer(0.03, self.callback) # 0.03 = 33FPS

    def callback(self):
        ret, frame = self.cap.read()
        if not ret:
            self.get_logger().error("Failed to read frame")
            return

        # Encodes image as JPG
        success, jpeg = cv2.imencode('.jpg', frame)
        if not success:
            return

        msg = CompressedImage()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.format = "jpeg"
        msg.data = jpeg.tobytes()

        self.pub.publish(msg)

def main():
    rclpy.init()
    node = GetImages()
    rclpy.spin(node)

if __name__ == '__main__':
    main()