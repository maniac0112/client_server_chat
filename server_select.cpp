#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <thread> 
#include <cstring>
#include <sys/select.h>
#include <unordered_set>

const int CLIENT_LIMIT = 100; 
const size_t MAX_BUFFER_SIZE = 4096; 
char read_buffer[MAX_BUFFER_SIZE];
char write_buffer[MAX_BUFFER_SIZE];

std::unordered_set<int> v_fds;
int server_socket = -1; 

std::queue<std::pair<std::string,int>> message_queue; 
std::mutex message_queue_mtx;
std::condition_variable message_notifier; 


void server_sender(){
    std::unique_lock<std::mutex> queue_lock(message_queue_mtx, std::defer_lock); 
    while(1){
        queue_lock.lock();
        if (!message_queue.empty()){ 
            //Since we have a single thread that pops, its safe to assume if we are non empty here, we would be non-empty throughout this thread before we perform pop()   
            std::string message;
            if (message_queue.front().second == 0){
                message = "Server: ";
            }

            message += std::move(message_queue.front().first);
            message_queue.pop();
            queue_lock.unlock();

            for (auto fd: v_fds){
                if (fd == server_socket || fd == STDIN_FILENO) continue;
                send(fd, message.data(), message.size(),0);
            }
            
        }else{
            message_notifier.wait(queue_lock);
            //I'll have the lock aquired again, unlock it 
            queue_lock.unlock();
        }
    } 
}

int main(){

    fd_set fds, readfds; 
    int n_fds = 0; 
    std::unique_lock<std::mutex> queue_lock(message_queue_mtx, std::defer_lock); 

    server_socket =  socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        perror("socket failed");
        return -1; 
    }

    FD_SET(server_socket, &fds);
    FD_SET(STDIN_FILENO, &fds);

    v_fds.insert(server_socket);
    v_fds.insert(STDIN_FILENO);

    n_fds = server_socket+1; 



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
    //Good Time To Get our server_worker to work
    std::thread message_broadcaster(server_sender); 
    message_broadcaster.detach();

    while(1){
        readfds = fds; 
        if (select(n_fds, &readfds, nullptr, nullptr, nullptr) < 0){
            perror("Select Failed, shutting down the server!  \n");
            return -1; 
        }

        for (auto it = v_fds.begin(); it!=v_fds.end(); it++){
            int fd = *it; 
            if (FD_ISSET(fd, &readfds)){
                if (fd == STDIN_FILENO){
                    ssize_t bytes_read = read(STDIN_FILENO, write_buffer, MAX_BUFFER_SIZE);
                    std::string server_message(write_buffer,bytes_read);
                    
                    //Broadcast this message to all clients 
                    queue_lock.lock();
                    message_queue.push({server_message,0});
                    queue_lock.unlock();
                    message_notifier.notify_all();
                    continue;

                }else if(fd == server_socket){
                    // Ready for Accepting a Connection!

                    int client_fd = accept(server_socket, nullptr, nullptr); //Make this Asynchronous! 
            
                    if (client_fd < 0){
                        std::cout<<"Failed to Accept a Connection.. Client might have terminated the connection \n"; 
                        continue;
                    }

                    std::cout<<"Connected to a New Client \n"; 
                    n_fds = ((n_fds > client_fd) ? n_fds : client_fd + 1);
                    FD_SET(client_fd, &fds);
                    v_fds.insert(client_fd);

                }else{

                    size_t bytes_received = read(fd, read_buffer, 1024); 
                    if (bytes_received <= 0){
                        v_fds.erase(fd); 
                        FD_CLR(fd, &fds);
                        n_fds = ((n_fds == fd + 1) ? n_fds-1 : n_fds);
                        std::cout<<"Connection Closed by Client"; 
                        continue;
                    }
                    std::string client_message(read_buffer,bytes_received);

                    if (client_message == "end"){
                        //Remove the client from the FD_SET and SET of clients 
                        v_fds.erase(fd); 
                        FD_CLR(fd, &fds);
                        n_fds = ((n_fds == fd + 1) ? n_fds-1 : n_fds);
                        std::cout<<"Connection Closed by Client"; 
                        continue;
                    
                    }

                    //Broadcast this message to all clients 
                    queue_lock.lock();
                    message_queue.push({client_message,1});
                    queue_lock.unlock();
                    message_notifier.notify_all();

                    std::cout<<client_message<<"\n";
                }
            }
        }
    }

}