#include "nalu_base/nalu_base_node.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <chrono>

using namespace std::chrono_literals;

namespace nalu_base
{

NaluBaseNode::NaluBaseNode(const rclcpp::NodeOptions & options)
: Node("nalu_base", options)
{
  declareParameters();
  loadParameters();

  odom_pub_    = create_publisher<nav_msgs::msg::Odometry>("odom", 10);
  battery_pub_ = create_publisher<sensor_msgs::msg::BatteryState>("battery", 10);
  status_pub_  = create_publisher<nalu_msgs::msg::Status>("status", 10);

  cmd_vel_sub_ = create_subscription<geometry_msgs::msg::Twist>(
    "cmd_vel", 10,
    std::bind(&NaluBaseNode::cmdVelCallback, this, std::placeholders::_1));

  tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

  if (openSerial()) {
    running_ = true;
    serial_thread_ = std::thread(&NaluBaseNode::serialReadLoop, this);
    RCLCPP_INFO(get_logger(), "✅ Serial aberta: %s @ %d baud", serial_port_.c_str(), serial_baud_);
  } else {
    RCLCPP_WARN(get_logger(), "⚠️  Serial não disponível: %s", serial_port_.c_str());
  }

  last_cmd_vel_time_ = now();
  last_odom_time_    = now();

  auto period = std::chrono::duration<double>(1.0 / publish_rate_hz_);
  publish_timer_ = create_wall_timer(
    std::chrono::duration_cast<std::chrono::nanoseconds>(period),
    std::bind(&NaluBaseNode::publishCallback, this));

  RCLCPP_INFO(get_logger(), "🤖 NaluBaseNode iniciado");
}

NaluBaseNode::~NaluBaseNode()
{
  running_ = false;
  if (serial_thread_.joinable()) serial_thread_.join();
  closeSerial();
  RCLCPP_INFO(get_logger(), "NaluBaseNode encerrado");
}

void NaluBaseNode::declareParameters()
{
  declare_parameter("serial_port",     "/dev/ttyACM0");
  declare_parameter("serial_baud",     115200);
  declare_parameter("max_linear_vel",  1.0);
  declare_parameter("max_angular_vel", 2.0);
  declare_parameter("cmd_vel_timeout", 0.5);
  declare_parameter("publish_rate_hz", 20.0);
  declare_parameter("odom_frame_id",   "odom");
  declare_parameter("base_frame_id",   "base_link");
  declare_parameter("wheel_base",      0.20);
}

void NaluBaseNode::loadParameters()
{
  serial_port_     = get_parameter("serial_port").as_string();
  serial_baud_     = get_parameter("serial_baud").as_int();
  max_linear_vel_  = get_parameter("max_linear_vel").as_double();
  max_angular_vel_ = get_parameter("max_angular_vel").as_double();
  cmd_vel_timeout_ = get_parameter("cmd_vel_timeout").as_double();
  publish_rate_hz_ = get_parameter("publish_rate_hz").as_double();
  odom_frame_id_   = get_parameter("odom_frame_id").as_string();
  base_frame_id_   = get_parameter("base_frame_id").as_string();
  wheel_base_      = get_parameter("wheel_base").as_double();
}

bool NaluBaseNode::openSerial()
{
  serial_fd_ = open(serial_port_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (serial_fd_ < 0) return false;

  struct termios tty;
  tcgetattr(serial_fd_, &tty);
  cfsetispeed(&tty, B115200);
  cfsetospeed(&tty, B115200);
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CREAD | CLOCAL;
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_oflag &= ~OPOST;
  tty.c_cc[VMIN]  = 0;
  tty.c_cc[VTIME] = 1;
  tcsetattr(serial_fd_, TCSANOW, &tty);
  serial_connected_ = true;
  return true;
}

void NaluBaseNode::closeSerial()
{
  if (serial_fd_ >= 0) { close(serial_fd_); serial_fd_ = -1; }
  serial_connected_ = false;
}

void NaluBaseNode::serialReadLoop()
{
  std::string buffer;
  char ch;
  while (running_) {
    int n = read(serial_fd_, &ch, 1);
    if (n < 0) { std::this_thread::sleep_for(1ms); continue; }
    if (n == 0) continue;
    if (ch == '\n') {
      if (!buffer.empty()) { parseTelemetry(buffer); buffer.clear(); }
    } else { buffer += ch; }
  }
}

static float parseFloat(const std::string & json, const std::string & key)
{
  auto pos = json.find("\"" + key + "\":");
  if (pos == std::string::npos) return 0.0f;
  pos += key.size() + 3;
  return std::stof(json.substr(pos));
}

static bool parseBool(const std::string & json, const std::string & key)
{
  auto pos = json.find("\"" + key + "\":");
  if (pos == std::string::npos) return false;
  pos += key.size() + 3;
  return json.substr(pos, 4) == "true";
}

void NaluBaseNode::parseTelemetry(const std::string & json_line)
{
  if (json_line.find("\"telemetry\"") == std::string::npos) return;
  std::lock_guard<std::mutex> lock(state_mutex_);
  battery_mv_     = parseFloat(json_line, "battery_mv");
  emergency_stop_ = parseBool(json_line,  "emergency_stop");
  motor_left_     = static_cast<int>(parseFloat(json_line, "motor_left"));
  motor_right_    = static_cast<int>(parseFloat(json_line, "motor_right"));
  RCLCPP_INFO(get_logger(), "Telemetria → bat: %.0f mV  e-stop: %d", battery_mv_, emergency_stop_);
}

void NaluBaseNode::sendCommand(double linear, double angular)
{
  if (serial_fd_ < 0) return;
  char buf[128];
  int len = snprintf(buf, sizeof(buf),
    "{\"type\":\"cmd\",\"linear\":%.3f,\"angular\":%.3f}\n", linear, angular);
  write(serial_fd_, buf, len);
}

void NaluBaseNode::cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
{
  last_cmd_vel_time_ = now();
  double linear  = std::clamp(msg->linear.x,  -max_linear_vel_,  max_linear_vel_);
  double angular = std::clamp(msg->angular.z, -max_angular_vel_, max_angular_vel_);
  last_linear_  = linear;
  last_angular_ = angular;
  sendCommand(linear, angular);
}

void NaluBaseNode::updateOdometry(double linear, double angular, double dt)
{
  theta_ += angular * dt;
  x_     += linear * std::cos(theta_) * dt;
  y_     += linear * std::sin(theta_) * dt;
}

void NaluBaseNode::publishCallback()
{
  auto now_time = now();
  double dt = (now_time - last_odom_time_).seconds();
  last_odom_time_ = now_time;

  double elapsed = (now_time - last_cmd_vel_time_).seconds();
  if (elapsed > cmd_vel_timeout_ && (last_linear_ != 0.0 || last_angular_ != 0.0)) {
    last_linear_ = last_angular_ = 0.0;
    sendCommand(0.0, 0.0);
  }

  updateOdometry(last_linear_, last_angular_, dt);

  tf2::Quaternion q;
  q.setRPY(0, 0, theta_);

  auto odom = nav_msgs::msg::Odometry();
  odom.header.stamp            = now_time;
  odom.header.frame_id         = odom_frame_id_;
  odom.child_frame_id          = base_frame_id_;
  odom.pose.pose.position.x    = x_;
  odom.pose.pose.position.y    = y_;
  odom.pose.pose.orientation.x = q.x();
  odom.pose.pose.orientation.y = q.y();
  odom.pose.pose.orientation.z = q.z();
  odom.pose.pose.orientation.w = q.w();
  odom.twist.twist.linear.x    = last_linear_;
  odom.twist.twist.angular.z   = last_angular_;
  odom_pub_->publish(odom);

  geometry_msgs::msg::TransformStamped tf_msg;
  tf_msg.header.stamp            = now_time;
  tf_msg.header.frame_id         = odom_frame_id_;
  tf_msg.child_frame_id          = base_frame_id_;
  tf_msg.transform.translation.x = x_;
  tf_msg.transform.translation.y = y_;
  tf_msg.transform.rotation.x    = q.x();
  tf_msg.transform.rotation.y    = q.y();
  tf_msg.transform.rotation.z    = q.z();
  tf_msg.transform.rotation.w    = q.w();
  tf_broadcaster_->sendTransform(tf_msg);

  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto bat = sensor_msgs::msg::BatteryState();
    bat.header.stamp = now_time;
    bat.voltage      = battery_mv_ / 1000.0f;
    bat.present      = true;
    battery_pub_->publish(bat);

    auto status = nalu_msgs::msg::Status();
    status.header.stamp      = now_time;
    status.is_connected      = serial_connected_;
    status.is_emergency_stop = emergency_stop_;
    status.battery_voltage   = battery_mv_ / 1000.0f;
    status.state             = emergency_stop_ ? "emergency_stop" : "running";
    status_pub_->publish(status);
  }
}

}  // namespace nalu_base

#include <rclcpp/rclcpp.hpp>
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<nalu_base::NaluBaseNode>());
  rclcpp::shutdown();
  return 0;
}
