#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/int16.hpp"

using namespace std::chrono_literals;

class MotorUdpNode : public rclcpp::Node {
public:
  MotorUdpNode() : Node("motor_udp_node") {
    // パブリッシャー (Teensyからのデータ出力用)
    status_pub_ = this->create_publisher<std_msgs::msg::String>("/motor_status", 10);

    // サブスクライバー (iPad/Webからの回転指示受信用)
    cmd_sub_ = this->create_subscription<std_msgs::msg::Int16>(
      "/cmd_rpm", 10, std::bind(&MotorUdpNode::cmd_callback, this, std::placeholders::_1));

    // UDPソケットの初期化
    init_udp();

    // 10ms周期でUDP受信チェック（ノンブロッキング）
    timer_ = this->create_wall_timer(10ms, std::bind(&MotorUdpNode::receive_udp, this));

    RCLCPP_INFO(this->get_logger(), "Motor UDP Node (双方向通信) を起動しました");
  }

  ~MotorUdpNode() {
    if (sockfd_ >= 0) close(sockfd_);
  }

private:
  void init_udp() {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    
    // ソケットをノンブロッキングモードに設定
    int flags = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);

    memset(&my_addr_, 0, sizeof(my_addr_));
    my_addr_.sin_family = AF_INET;
    my_addr_.sin_addr.s_addr = INADDR_ANY;
    my_addr_.sin_port = htons(8888); // PC側の受信用ポート

    bind(sockfd_, (struct sockaddr*)&my_addr_, sizeof(my_addr_));

    // Teensyのアドレス設定
    memset(&teensy_addr_, 0, sizeof(teensy_addr_));
    teensy_addr_.sin_family = AF_INET;
    teensy_addr_.sin_port = htons(8888);
    inet_pton(AF_INET, "192.168.1.10", &teensy_addr_.sin_addr); // Teensy IP
  }

  // A. TeensyからのUDP受信＆ROS 2トピックへパブリッシュ
  void receive_udp() {
    char buffer[256];
    ssize_t bytes_received = recv(sockfd_, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
      buffer[bytes_received] = '\0';
      auto msg = std_msgs::msg::String();
      msg.data = std::string(buffer);
      status_pub_->publish(msg);
    }
  }

  // B. ROS 2トピック(/cmd_rpm)を受信したらTeensyへUDP送信
  void cmd_callback(const std_msgs::msg::Int16::SharedPtr msg) {
    std::string send_str = std::to_string(msg->data);
    sendto(sockfd_, send_str.c_str(), send_str.length(), 0,
           (struct sockaddr*)&teensy_addr_, sizeof(teensy_addr_));
    RCLCPP_INFO(this->get_logger(), "Teensyへ目標RPM送信: %d", msg->data);
  }

  int sockfd_{-1};
  struct sockaddr_in my_addr_, teensy_addr_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
  rclcpp::Subscription<std_msgs::msg::Int16>::SharedPtr cmd_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MotorUdpNode>());
  rclcpp::shutdown();
  return 0;
}