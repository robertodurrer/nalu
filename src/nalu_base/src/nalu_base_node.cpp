#include "nalu_base/nalu_base_node.hpp"
#include <chrono>

using namespace std::chrono_literals;

namespace nalu_base
{

NaluBaseNode::NaluBaseNode(const rclcpp::NodeOptions & options)
: Node("nalu_base", options)
{
  declareParameters();
  loadParameters();

  // Publishers
  odom_pub_    = create_publisher<nav_msgs::msg::Odometry>("odom", 10);
  battery_pub_ = create_publisher<sensor_msgs::msg::BatteryState>("battery", 10);
  status_pub_  = create_publisher<nalu_msgs::msg::Status>("status", 10);

  // Subscriber
  cmd_vel_sub_ = create_subscription<geometry_msgs::msg::Twist>(
    "cmd_vel", 10,
    std::bind(&NaluBaseNode::cmdVelCallback, this, std::placeholders::_1));

  // TF broadcaster
  tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

  // Timer de publicação
  auto period = std::chrono::duration<double>(1.0 / publish_rate_hz_);
  publish_timer_ = create_wall_timer(
    std::chrono::duration_cast<std::chrono::nanoseconds>(period),
    std::bind(&NaluBaseNode::publishCallback, this));

  last_cmd_vel_time_ = now();

  RCLCPP_INFO(get_logger(), "✅ NaluBaseNode iniciado");
  RCLCPP_INFO(get_logger(), "   Porta serial: %s @ %d baud", serial_port_.c_str(), serial_baud_);
}

NaluBaseNode::~NaluBaseNode()
{
  RCLCPP_INFO(get_logger(), "NaluBaseNode encerrado");
}

void NaluBaseNode::declareParameters()
{
  declare_parameter("serial_port",     "/dev/ttyUSB0");
  declare_parameter("serial_baud",     115200);
  declare_parameter("max_linear_vel",  1.0);
  declare_parameter("max_angular_vel", 2.0);
  declare_parameter("cmd_vel_timeout", 0.5);
  declare_parameter("publish_rate_hz", 50.0);
  declare_parameter("odom_frame_id",   "odom");
  declare_parameter("base_frame_id",   "base_link");
}

void NaluBaseNode::loadParameters()
{
  serial_port_      = get_parameter("serial_port").as_string();
  serial_baud_      = get_parameter("serial_baud").as_int();
  max_linear_vel_   = get_parameter("max_linear_vel").as_double();
  max_angular_vel_  = get_parameter("max_angular_vel").as_double();
  cmd_vel_timeout_  = get_parameter("cmd_vel_timeout").as_double();
  publish_rate_hz_  = get_parameter("publish_rate_hz").as_double();
  odom_frame_id_    = get_parameter("odom_frame_id").as_string();
  base_frame_id_    = get_parameter("base_frame_id").as_string();
}

void NaluBaseNode::cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
{
  last_cmd_vel_time_ = now();

  // Clamp velocidades
  double linear  = std::clamp(msg->linear.x,  -max_linear_vel_,  max_linear_vel_);
  double angular = std::clamp(msg->angular.z, -max_angular_vel_, max_angular_vel_);

  RCLCPP_DEBUG(get_logger(), "cmd_vel → linear: %.2f  angular: %.2f", linear, angular);

  // TODO: enviar comandos para o hardware via serial
  (void)linear;
  (void)angular;
}

void NaluBaseNode::publishCallback()
{
  checkCmdVelTimeout();

  auto stamp = now();

  // --- Odometria (placeholder) ---
  auto odom = nav_msgs::msg::Odometry();
  odom.header.stamp    = stamp;
  odom.header.frame_id = odom_frame_id_;
  odom.child_frame_id  = base_frame_id_;
  odom_pub_->publish(odom);

  // --- Status ---
  auto status = nalu_msgs::msg::Status();
  status.header.stamp = stamp;
  status.is_connected = true;   // TODO: checar conexão serial real
  status_pub_->publish(status);
}

void NaluBaseNode::checkCmdVelTimeout()
{
  double elapsed = (now() - last_cmd_vel_time_).seconds();
  if (elapsed > cmd_vel_timeout_) {
    // TODO: parar motores
  }
}

}  // namespace nalu_base

// Main
#include <rclcpp/rclcpp.hpp>
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<nalu_base::NaluBaseNode>());
  rclcpp::shutdown();
  return 0;
}
