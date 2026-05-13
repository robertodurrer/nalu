#pragma once

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <sensor_msgs/msg/battery_state.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <diagnostic_updater/diagnostic_updater.hpp>
#include "nalu_msgs/msg/status.hpp"

namespace nalu_base
{

class NaluBaseNode : public rclcpp::Node
{
public:
  explicit NaluBaseNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~NaluBaseNode();

private:
  // Parâmetros
  std::string serial_port_;
  int serial_baud_;
  double max_linear_vel_;
  double max_angular_vel_;
  double cmd_vel_timeout_;
  double publish_rate_hz_;
  std::string odom_frame_id_;
  std::string base_frame_id_;

  // Publishers
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  rclcpp::Publisher<sensor_msgs::msg::BatteryState>::SharedPtr battery_pub_;
  rclcpp::Publisher<nalu_msgs::msg::Status>::SharedPtr status_pub_;

  // Subscribers
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;

  // TF
  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

  // Timers
  rclcpp::TimerBase::SharedPtr publish_timer_;
  rclcpp::Time last_cmd_vel_time_;

  // Callbacks
  void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg);
  void publishCallback();

  // Helpers
  void declareParameters();
  void loadParameters();
  void checkCmdVelTimeout();
};

}  // namespace nalu_base
