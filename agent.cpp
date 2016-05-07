// Assignment 2
// The Ticket Counter

// Objective
// To simulate a railway ticket booking system where several ticket booking agents (clients) simultaneously 
// book tickets by sending requests to the central server.

// Group Details
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)

// Filename: agent.cpp -- Implementation of Booking Agent

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <iomanip>
#include <sys/time.h>

#define MESSAGE_LENGTH 50000

using namespace std;

int PORT_NO;


#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

class CSVRow
{
    public:
        string const& operator[](size_t index) const
        {
            return m_data[index];
        }
        size_t size() const
        {
            return m_data.size();
        }
        void readNextRow(istream& str)
        {
            string line;
            getline(str,line);

            stringstream lineStream(line);
            string cell;

            m_data.clear();
            while(getline(lineStream,cell,','))
            {
                m_data.push_back(cell);
            }
        }
    private:
        vector<string> m_data;
};

istream& operator>>(std::istream& str,CSVRow& data)
{
    data.readNextRow(str);
    return str;
}


// Function foe receiving Message
string receive_msg(int connfd,int bytes)
{
    string output;
    output.clear();
    output.resize(bytes);

    int bytes_received = read(connfd, &output[0], bytes-1);
    if (bytes_received<0)
    {
        std::cerr << "Failed to read data from socket.\n";
        return NULL;
    }

    output.resize(bytes_received);
    char* cstr = new char[output.length() +1];
    strcpy(cstr, output.c_str());
    string msg(cstr);
    cout<<setiosflags(ios::left);
    cout<<"Message Received: \n"<<msg;
    return msg;
}

// Function for sending Message
void send_msg(int connfd,string dataToSend)
{

    uint32_t dataLength = htonl(dataToSend.size()); // Ensure network byte order
    // when sending the data length
    //send(connfd,&dataLength ,sizeof(uint32_t) ,MSG_CONFIRM); // Send the data length
    send(connfd,dataToSend.c_str(),dataToSend.size(),MSG_CONFIRM); // Send the string
}

int main(int argc, char * argv[])
{
	PORT_NO = atoi(argv[2]);
    int agentFD = socket(AF_INET,SOCK_STREAM,0),n;
    if(agentFD < 0)printf("Socket formation Failed\n");

    struct sockaddr_in saddr;
    printf("Tryint to Connect ...\n");

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT_NO);

    if(inet_pton(AF_INET,argv[1],&saddr.sin_addr) <= 0)printf("Addr conv Failed\n");

    if(connect(agentFD,(struct  sockaddr*)&saddr,sizeof(saddr)) < 0)printf("Connection Failed\n");
    printf("\033[H\033[J");
    printf("Connection established ...\n");

    string sendmsg;
    string recmsg;
    string path;
    int flag = 0;

    while(1)
    {
    	if(flag==1)
    	{
    		printf("\033[H\033[J");
    	}

        // Receive the Menu
    	recmsg = receive_msg(agentFD,MESSAGE_LENGTH);

        // Send the option
    	sendmsg.clear();
    	cin>>sendmsg;
    	send_msg(agentFD,sendmsg+"\t");

        // If You want to get Train Status
    	if(sendmsg=="1")
    	{
    		recmsg = receive_msg(agentFD,MESSAGE_LENGTH);
    		sendmsg.clear();
    		cin>>sendmsg;
    		send_msg(agentFD,"5\t"+sendmsg);
    		recmsg = receive_msg(agentFD,MESSAGE_LENGTH);
    		send_msg(agentFD,"Sucessfully received");
    	}

        // If you want to book tickets
    	else if(sendmsg == "2")
    	{
    		recmsg = receive_msg(agentFD,MESSAGE_LENGTH);
    		sendmsg.clear();
    		cin>>path;
    		ifstream bookingFile(path.c_str());
    		CSVRow row;
    		stringstream rows;
    		rows.str(std::string());
            struct timeval tp;
            gettimeofday(&tp, NULL);
            long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

            int cnt = 0;

		    while(bookingFile >> row)
		    {
		    	rows<<ms<<"\t"<<row[0]<<"\t"<<row[1]<<"\t"<<row[2]<<"\t"<<row[3]<<"\t"<<row[4]<<"\t"<<row[5]<<endl;
                cnt++;
		    }
		    send_msg(agentFD,"4\t"+rows.str());
            ofstream bookingOutput((path+"_Output.csv").c_str());
            while(cnt--){
		        recmsg = receive_msg(agentFD,MESSAGE_LENGTH);
                bookingOutput<<recmsg;
		        send_msg(agentFD,"Sucessfully received");
            }   
            bookingOutput.close();
    	}

        // If you want to exit
    	else if(sendmsg == "3")
    	{
    		break;
    	}
    	recmsg = receive_msg(agentFD,MESSAGE_LENGTH);
    	sendmsg.clear();
    	cin>>sendmsg;
    	send_msg(agentFD,"6\t"+sendmsg);
    	if(sendmsg == "n") break;

    	flag = 1;
    }
}
