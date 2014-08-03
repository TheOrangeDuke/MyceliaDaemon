#include "../include/CMSClient.h"
#include <errno.h>

CMSClient::CMSClient()
{
    memset(&conn.host_info, 0, sizeof conn.host_info);
}

CMSClient::CMSClient(const char* serverIP, const char* serverPort)
{
    conn.serverIP = serverIP;
    conn.serverPort = serverPort;
    memset(&conn.host_info, 0, sizeof conn.host_info);
}

CMSClient::~CMSClient()
{

}

void CMSClient::setupServerInfo(const char* serverIP, const char* serverPort)
{
    conn.serverIP = serverIP;
    conn.serverPort = serverPort;
}

int CMSClient::findServer()
{
    bool seekingServer = true;
    const char* hostname = "localhost";
    const char* portname = "8888";

    int socketfd = 0;
    struct sockaddr_in dConn;
    struct sockaddr_in src_addr;
    memset(&dConn, 0, sizeof(dConn));

    dConn.sin_family = AF_INET;
    dConn.sin_port = htons(8888);
    dConn.sin_addr.s_addr = INADDR_ANY;

    int broadcast = 1;

    //if (getaddrinfo(hostname,portname,&dConn,&dConnp) != 0)
    //    std::cout << "failed to resolve hostname: " << std::endl;

    if ((socketfd = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP )) == -1)
        std::cout << "failed to create socket " << strerror(socketfd) << std::endl;

    setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);

    if ((bind(socketfd, (sockaddr *)&dConn, sizeof(sockaddr))) == -1)
        std::cout << "failed to bind socket" << std::endl;


    while (seekingServer)
    {
        char buffer[255];
        memset(&buffer, 0, sizeof(buffer));
        // socklen_t src_addr_len=sizeof(src_addr);
        unsigned slen=sizeof(sockaddr);
        ssize_t count=recvfrom(socketfd,buffer,sizeof(buffer),0,(sockaddr *)&src_addr,&slen);
        if (count==-1)
        {
            printf("none received\n");
        }
        else if (count==sizeof(buffer))
        {
            printf("datagram too large for buffer");
        }
        else
        {
            printf("recv: %s\n", buffer);
            //
            int delimit = 0;
            int eos = 0;
            for (int i = 0; i < 255; i++)
            {
                if (buffer[i] == ':')
                {
                    delimit = i;
                }
                else if (buffer[i] == '\r')
                {
                    eos = i;
                }

            }
            std::string ipinfo(buffer);
            std::string ip = ipinfo.substr(0, delimit);
            std::string port =  ipinfo.substr(delimit + 1,(eos - delimit) - 1);
            conn.serverIP = ip.c_str();
            conn.serverPort = port.c_str();
            std::cout << "serverIP: |" << conn.serverIP << "| port: |" << conn.serverPort << "|" << std::endl;
            seekingServer = false;

        }

        sleep(1);
    }

 close(socketfd);
    return 0;

}

int CMSClient::connectToServer()
{
    std::cout << "TEST " << conn.serverIP << " WITH PORT: " << conn.serverPort << std::endl;
    // Struct Setup Protocol
    std::cout << "Setting up the structs..." << std::endl;
    conn.host_info.ai_family = AF_INET;
    conn.host_info.ai_socktype = SOCK_STREAM;
    conn.host_info.ai_protocol = IPPROTO_TCP;
    std::cout << "addr ai " << conn.serverIP << " " << conn.serverPort << std::endl;
    //conn.status = getaddrinfo(conn.serverIP, conn.serverPort, &conn.host_info, &conn.host_info_list); //"IP", "port", leave it alone, leave it alone
    conn.status = getaddrinfo("132.205.84.236", "6969", &conn.host_info, &conn.host_info_list); //"IP", "port", leave it alone, leave it alone
    if(conn.status !=0)
    {
        std::cout << "..STATUS CODE: " << conn.status << std::endl;
        std::cout << "..Err:" << gai_strerror(conn.status) << std::endl;
    }
    else
    {
        std::cout << "..Struts setup returned: " << conn.status << "[OK]" << std::endl;
    }

    // Socket Creation Protocol
    std::cout << "Creating a socket..." << std::endl;
    std::cout << "Setting up the socket..." << std::endl;
    socketfd = socket(conn.host_info_list->ai_family, conn.host_info_list->ai_socktype,
                      conn.host_info_list->ai_protocol);
    if(socketfd == -1)
    {
        std::cout << "..Socket Error"<< std::endl;
    }
    else
    {
        std::cout << "..Socket setup returned: " << socketfd << "[OK]" << std::endl;
    }

    // Socket Connection Protocol
    std::cout << "Connecting to server" << std::endl;
    conn.status = connect(socketfd, conn.host_info_list->ai_addr, conn.host_info_list->ai_addrlen);
    if(conn.status == -1)
    {
        std::cout << "..Connection Error"<< std::endl;
    }
    else
    {
        std::cout << "..Connected to host at: " << conn.serverIP << "[OK]" << std::endl;
    }

    return 0;
}

std::string CMSClient::execAndStore(char* cmd)
{
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe))
    {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}

std::string CMSClient::createTestXMLPayload(systemStats *ss)
{
    std::string payload;
    // payload.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    payload.append("<transmission>");

    payload.append("<header id=\"1\" type=\"sys\" to=\"cms\" from=\"node1\">");
    payload.append("OPCODE");
    payload.append("</header>");

    payload.append("<particle type=\"sys\" class=\"temperature\" count=\"1\">");
    payload.append(ss->systemTemp);
    payload.append("</particle>");

    payload.append("<particle type=\"sys\" class=\"temperature\" count=\"1\">");
    payload.append(ss->systemTemp);
    payload.append("</particle>");

    payload.append("</transmission>\r\n");

    return payload;
}


void CMSClient::updateSystemStats(systemStats *ss)
{
    //###The full output of a command ran on the system (assign with execAndStore)
    std::string commandRecv;
    //###Formatted command to comply with conversion to other data types.
    std::string commandFormatted;

    //Update this particular nodes temperature (in C because we arent F[uckheads])
    commandRecv = execAndStore("/opt/vc/bin/vcgencmd measure_temp");
    commandFormatted = commandRecv.substr(5,4);
    ss->systemTemp = commandFormatted;
    //std::cout << "CF: " << commandFormatted << std::endl;
    //ss->systemTemp = ::atof(commandFormatted.c_str());
    std::cout << "SS TEMP VAL: " << ss->systemTemp << std::endl;
}

int CMSClient::sendSystemInfo(systemStats *ss)
{

    // Data Sending Protocol
    std::cout << "Sending XML payload...";
    ssize_t bytes_sent;
    std::string payload = createTestXMLPayload(ss);
    std::string *payloadP = &payload;
    std::cout << "MEGA TEST2: " << payload << std::endl;
    //const char* payloadConverted = payload.c_str();
    bytes_sent = send(socketfd, payload.c_str(), strlen(payload.c_str()), 0);
    std::cout << " Sent: " << bytes_sent << std::endl;

    return 0;
}
/*
sockaddr_in si_client, si_server;
    int socketD;
    int broadcast = 1;

    if ((socketD=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
        perror("socket");
    } else {
        std::cout << "Started listening on socket 8888" << std::endl;
    }

    memset(&si_client, 0, sizeof(si_client));
    si_client.sin_family = AF_INET;
    si_client.sin_port = htons(8888);
    si_client.sin_addr.s_addr = INADDR_ANY;


    setsockopt(socketD, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);

    while(1)
    {
        char buffer[10000];
        unsigned slen = sizeof(sockaddr);
        std::cout << "HERE" << std::endl;
        if (recvfrom(socketD, buffer, sizeof(buffer)-1, 0, (sockaddr *)&si_server, &slen)==-1) {
            perror("receive");
        } else {
            printf("recv: %s\n", buffer);
        }

        sleep(1);
    }

    return 0;
*/
