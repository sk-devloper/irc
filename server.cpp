#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <map>

std::atomic<int> client_counter(0);
std::mutex clients_mutex;
std::map<int, std::string> client_names; // client_socket -> username
std::atomic<bool> running(true);
std::vector<int> clients;

void broadcast_message(const std::string &msg, int sender_socket = -1) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int client_sock : clients) {
        if (client_sock != sender_socket) {
            if (send(client_sock, msg.c_str(), msg.size(), 0) <= 0) {
                close(client_sock);
                client_names.erase(client_sock);
                clients.erase(std::remove(clients.begin(), clients.end(), client_sock), clients.end());
            }
        }
    }
}

void handle_client(int client_socket, int client_id) {
    // Ask client for a username
    const char* ask_name = "Enter your username: ";
    send(client_socket, ask_name, strlen(ask_name), 0);

    char name_buf[50];
    int name_len = recv(client_socket, name_buf, sizeof(name_buf) - 1, 0);
    if (name_len <= 0) {
        close(client_socket);
        return;
    }
    name_buf[name_len-1] = '\0'; // remove newline
    std::string username = name_buf;

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(client_socket);
        client_names[client_socket] = username;
    }

    // Send welcome message
    std::string welcome = "Welcome " + username + "! You are connected to the server.\n";
    send(client_socket, welcome.c_str(), welcome.size(), 0);

    // Notify others
    std::string join_msg = username + " has joined the chat.\n";
    std::cout << join_msg;
    broadcast_message(join_msg, client_socket);

    char buffer[1024];
    int bytes_received;
    while (running && (bytes_received = recv(client_socket, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        std::string msg = username + ": " + buffer;
        std::cout << msg;
        broadcast_message(msg, client_socket);
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
        client_names.erase(client_socket);
    }

    close(client_socket);

    // Notify others about disconnect
    std::string leave_msg = username + " has left the chat.\n";
    std::cout << leave_msg;
    broadcast_message(leave_msg);
}

int main() {
    int server_port = 12345;
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) { perror("Socket failed"); return 1; }

    // Reuse address
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed"); return 1;
    }

    if (listen(server_socket, 10) < 0) { perror("Listen failed"); return 1; }

    std::cout << "Server listening on port " << server_port << "\n";

    while (running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) { perror("Accept failed"); continue; }

        int client_id = ++client_counter;
        std::thread(handle_client, client_socket, client_id).detach();
    }

    close(server_socket);
    return 0;
}
