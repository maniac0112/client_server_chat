#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <cstring>
#include <thread>

const size_t MAX_BUFFER_SIZE = 4096; 
char read_buffer[MAX_BUFFER_SIZE];
char write_buffer[MAX_BUFFER_SIZE];
std::string chat_name; 

int client_socket = -1; 

void client_sender(){
    while(1){
        ssize_t bytes_read = read(STDIN_FILENO, write_buffer, MAX_BUFFER_SIZE);
        std::string client_message = chat_name + ": " + std::string(write_buffer,bytes_read);

        if (client_message == "end"){
            close(client_socket);
            return ;
        }

        send(client_socket, client_message.data(), client_message.size(),0);
    }
}

void client_receiver(){
    while(1){
        ssize_t bytes_received = read(client_socket, read_buffer, MAX_BUFFER_SIZE); 
        if (bytes_received <= 0){
            std::cout<<"Connection is broken \n";
            close(client_socket);
            return;  
        }

        std::string incoming_message(read_buffer, bytes_received); 
        std::cout<<incoming_message<<std::endl;
    }
}

int main(){
    
    std::cout<<"Please Enter Your Chat Name \n"; 
    std::cin>>chat_name; 

    client_socket =  socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr; 
    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(1100);
    if (inet_pton(AF_INET, "0.0.0.0", &server_addr.sin_addr) <= 0){
        perror("Failed to parse server address!");
        close(client_socket);
        return -1; 
    }

    size_t serv_addr_len = sizeof(server_addr);

    if (connect(client_socket, (sockaddr *) &server_addr, serv_addr_len) < 0){
        perror("Failed to Connect to Server!");
        close(client_socket);
        return -1; 
    }

    std::cout<<" connected to the server \n";
    std::thread cs(client_sender);
    std::thread cr(client_receiver);
    cs.join();
    cr.join();
}