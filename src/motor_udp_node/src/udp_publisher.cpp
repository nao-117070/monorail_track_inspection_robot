#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std::chrono_literals;

class MotorUdpNode : public rclcpp::Node {
public:
    MotorUdpNode() : Node("motor_udp_node") {
        // 1. "motor_status" という名前の掲示板（トピック）を作る
        publisher_ = this->create_publisher<std_msgs::msg::String>("motor_status", 10);

        // 2. UDP通信の準備
        setupUdp();

        // 3. 10ミリ秒ごとに「データが来てるかな？」と確認するタイマー
        timer_ = this->create_wall_timer(
            10ms, std::bind(&MotorUdpNode::receiveData, this));
            
        RCLCPP_INFO(this->get_logger(), "UDP受信＆ROS 2パブリッシュを開始！(ポート: 8888)");
    }

    ~MotorUdpNode() {
        if (sockfd_ >= 0) close(sockfd_);
    }

private:
    int sockfd_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;

    void setupUdp() {
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        
        // データが来ていなくてもプログラムが止まらないようにする（ノンブロッキング設定）
        fcntl(sockfd_, F_SETFL, O_NONBLOCK);

        struct sockaddr_in servaddr{};
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(8888);

        bind(sockfd_, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    }

    void receiveData() {
        char buffer[1024];
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);

        // UDPデータを受信
        int n = recvfrom(sockfd_, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliaddr, &len);
        
        if (n > 0) {
            buffer[n] = '\0'; // 文字列の終わり

            // ROS 2のメッセージ箱に入れて、掲示板にパブリッシュ！
            auto message = std_msgs::msg::String();
            message.data = std::string(buffer);
            publisher_->publish(message);

            // 画面にも表示
            RCLCPP_INFO(this->get_logger(), "Teensyから受信 -> トピックへ送信: '%s'", message.data.c_str());
        }
    }
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MotorUdpNode>());
    rclcpp::shutdown();
    return 0;
}