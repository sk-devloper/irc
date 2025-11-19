#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic>

std::atomic<bool> running(true);

void receive_messages(int sock) {
    char buffer[1024];
    while (running) {
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << buffer << std::flush;
        } else if (bytes_received == 0) {
            std::cout << "\n[Server disconnected]\n";
            running = false;
            break;
        } else {
            perror("recv failed");
            running = false;
            break;
        }
    }
}

int main() {
    const char* server_ip = "172.111.50.86"; // VPS public IP
    int server_port = 12345;

    std::cout << "Creating socket...\n";
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("Socket failed"); return 1; }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address"); return 1;
    }

    std::cout << "Connecting to " << server_ip << ":" << server_port << "...\n";
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        close(sock);
        return 1;
    }
    std::cout << "Connected!\n";

    std::thread recv_thread(receive_messages, sock);

    // Handle username prompt
    char buffer[1024];
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::cout << buffer << std::flush; // "Enter your username: "
    }

    std::string username;
    std::getline(std::cin, username);
    username += "\n";
    send(sock, username.c_str(), username.size(), 0);

    // Continue receiving welcome message
    bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::cout << buffer << std::flush; // "Welcome X! You are connected..."
    }

    // Main input loop
    std::string input;
    while (running) {
        if (!std::getline(std::cin, input)) {
            running = false;
            break;
        }

        if (!running) break;
        if (input == "exit") break;

        input += "\n";
        if (send(sock, input.c_str(), input.size(), 0) <= 0) {
            std::cerr << "[Send failed]\n";
            running = false;
            break;
        }
    }

    running = false;
    shutdown(sock, SHUT_RDWR);
    close(sock);
    if (recv_thread.joinable()) recv_thread.join();

    std::cout << "[Client exited]\n";
    return 0;
}
