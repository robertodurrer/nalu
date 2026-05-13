#pragma once
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <sensor_msgs/msg/battery_state.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2/LinearMath/Quaternion.h>
#include "nalu_msgs/msg/status.hpp"
#include <termios.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>

namespace nalu_base
{
class NaluBaseNode : public rclcpp::Node
{
public:
  explicit NaluBaseNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~NaluBaseNode();
private:
  std::string serial_port_; int serial_baud_;
  double max_linear_vel_, max_angular_vel_, cmd_vel_timeout_, publish_rate_hz_;
  std::string odom_frame_id_, base_frame_id_;
  double wheel_base_;
  int serial_fd_{-1};
  std::thread serial_thread_;
  std::atomic<bool> running_{false};
  std::mutex state_mutex_;
  float battery_mv_{0.0f};
  bool emergency_stop_{false};
  int motor_left_{0}, motor_right_{0};
  bool serial_connected_{false};
  double x_{0.0}, y_{0.0}, theta_{0.0};
  double last_linear_{0.0}, last_angular_{0.0};
  rclcpp::Time last_odom_time_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  rclcpp::Publisher<sensor_msgs::msg::BatteryState>::SharedPtr battery_pub_;
  rclcpp::Publisher<nalu_msgs::msg::Status>::SharedPtr status_pub_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
  rclcpp::TimerBase::SharedPtr publish_timer_;
  rclcpp::Time last_cmd_vel_time_;
  void declareParameters();
  void loadParameters();
  bool openSerial();
  void closeSerial();
  void serialReadLoop();
  void parseTelemetry(const std::string & json_line);
  void sendCommand(double linear, double angular);
  void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg);
  void publishCallback();
  void updateOdometry(double linear, double angular, double dt);
};
}
