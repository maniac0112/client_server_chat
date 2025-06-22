#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <thread> 
#include <cstring>



const int CLIENT_LIMIT = 100; 
const size_t MAX_BUFFER_SIZE = 4096; 

void server_sender(int client_socket){
    
    char read_buffer[MAX_BUFFER_SIZE];
    char write_buffer[MAX_BUFFER_SIZE];

    while(1){
        size_t bytes_received = read(client_socket, read_buffer, 1024); 
        std::string client_message(read_buffer,bytes_received);

        if (client_message == "end"){
            std::cout<<"Connection Closed by Client"; 
            break; 
        }

        std::cout<<"Client Response: "<<client_message<<std::endl;
        std::cout<<"Send message to your client: \n";


        std::cin.getline(write_buffer, sizeof(write_buffer));

        size_t bytes_to_send = strlen(write_buffer);
        ssize_t sent_bytes = send(client_socket, write_buffer, bytes_to_send,0); 

        if (sent_bytes<0){
            perror("Sending Message failed, closing connection with client");
            close(client_socket);
        }
    }

    close(client_socket);
}

 
int main(){

    int server_socket =  socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        perror("socket failed");
        return -1; 
    }

    sockaddr_in server_address; 
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(1100);
    if (inet_pton(AF_INET, "0.0.0.0", &server_address.sin_addr) <=0 ){
        perror("Failed to convert the given address to a usable binary format");
        close(server_socket);
        return 0; 
    }

    size_t server_address_size = sizeof(server_address);

    //Bind the Socket to the given port
    if (bind(server_socket, reinterpret_cast<const sockaddr*>(&server_address), server_address_size) < 0){
        perror("Failed to bind the given IP and Port to the server socket");
        close(server_socket);
        return 0; 
    }

    if (listen(server_socket,CLIENT_LIMIT) < 0){
        perror("Failed to listen from the server socket");
        close(server_socket);
        return 0;
    } 

    std::cout<<"Server Started Listening to Incoming Connections in 0.0.0.0::1100 \n";

    while(true){
        int client_socket = accept(server_socket, nullptr, nullptr);   
        if (client_socket<0){
            if (client_socket < 0) {
                perror("Failed to accept connection\n");
                continue;
            }else{
                std::cout<< "Connected to a Client! \n"; 
            }
        }

        std::thread client_thread(server_worker, client_socket);
        client_thread.detach();
    }

}