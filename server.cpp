// Assignment 2
// The Ticket Counter

// Objective
// To simulate a railway ticket booking system where several ticket booking agents (clients) simultaneously 
// book tickets by sending requests to the central server.

// Group Details
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)

// Filename: server.cpp -- Implementation of Server

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <vector>
#include <map>
#include <iomanip>
#include <set>

using namespace std;


#define NO_OF_CLIENTS 20
#define MESSAGE_LENGTH 50000
#define PACKET_SIZE 8
#define MAX_NUM_OF_TRAINS 1000

int PNR_Count = 10;
int PORT_NO,PORT_NO_r,PORT_NO_a;
int liveThreads;
string path = "trains_new.txt";

// Struct Of Coach
typedef struct Coach
{
    int num_seats;	// Number of seats in a coach
    vector<int> num_available;	// Availability of seats
} Coach;


// struct of class of a coach
typedef struct _Class
{
    map<string,Coach> coach;	// <Coach No., CoachInfo>
} Class;


// struct of Train
typedef struct Train
{
    string name;	// Name of the Train
    Class AC;		// AC Class Info
    Class SL;		// Sleeper Class Info
} Train;


// struct of Seat Info
typedef struct SeatInfo{
	int seatNo;	// Seat Number of the Passenger
	int age;	// Age Of the Passenger
}seatInfo;

// struct of Priority to be given to Booking
typedef struct Priority{
	int agentFD;	// ConnFD og the agent
	string passId;	// Booking ID
	long int timestamp;	// Timestamp of receiving the request in milliseconds
	int no_of_seniors;	// Number of Seniors
	int no_of_passengers;	// Number of Passengers
}Priority;


// struct of Ticket
typedef struct Ticket
{
    string trainNo;	// Train Number
    string coachType;	// Type Of Coach
    int num_berths;	// Number Of Berths
    vector<string> preferences;	// Preferences Given
    vector<int> ages;	// Ages Of Passengers
    multimap<string,seatInfo> seat;	// <Coach No, Seat Info>
} Ticket;


// Table of Train Number and Train
map<string,Train> trains;

// Table of PNR Number and Ticket
map<int,Ticket> tickets;


// Comparator for Priortizing the requests
struct Comp{
bool operator()(Priority priority1,Priority priority2) {

	// First prioritize requests which came earlier
    if(priority1.timestamp < priority2.timestamp) return true;
    else if(priority1.timestamp > priority2.timestamp) return false;

    // Then prioritize requests with more number of senior citizens
    if(priority1.no_of_seniors > priority2.no_of_seniors) return true;
    else if(priority1.no_of_seniors < priority2.no_of_seniors) return false;

    // Finally prioritize requests with more number of passengers
    if(priority1.no_of_passengers > priority2.no_of_passengers) return true;
    else if(priority1.no_of_passengers < priority2.no_of_passengers) return false;

    return true;
}
};


// Table of Priority and Ticket for each request
multimap<Priority,Ticket,Comp> booking_queue;


// receive_msg Function
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
    cout<<"Message Received: \n"<<msg<<endl;
    return msg;
}

// Send_msg Function
void send_msg(int connfd,string dataToSend)
{

    uint32_t dataLength = htonl(dataToSend.size()); // Ensure network byte order
    // when sending the data length
    //send(connfd,&dataLength ,sizeof(uint32_t) ,MSG_CONFIRM); // Send the data length
    send(connfd,dataToSend.c_str(),dataToSend.size(),MSG_CONFIRM); // Send the string
}

// Function for updating the Train Info in the File
void updateFile()
{
    ofstream trainFile;
    stringstream out;
    trainFile.open(path.c_str(),ios::out|ios::trunc);
    map<string,Train>::iterator itr;
    itr = trains.begin();

    //cout<<"In Update  "<<trains.size()<<endl;

    while(itr!=trains.end())
    {
        //cout<<"In While"<<endl;
        trainFile<<(itr->first)<<"\t";
        trainFile<<((itr->second).name)<<"\t";
        trainFile<<((itr->second).AC.coach.size())<<"\t";
        if((itr->second).AC.coach.size() == 0)
        {
            trainFile<<"0\t\t ";
        }
        else
        {
            //cout<<"In else"<<endl;
            trainFile<<((itr->second).AC.coach)["A-1"].num_seats<<"\t";
            for(int i=1; i<=(itr->second).AC.coach.size(); ++i)
            {
                out.str(std::string());
                out<<i;

                for(int j = 0; j < (((itr->second).AC.coach)["A-"+out.str()].num_available).size(); ++j)
                    trainFile<<(((itr->second).AC.coach)["A-"+out.str()].num_available)[j]<<" ";

                //cout<<"After while"<<endl;

            }
            trainFile<<"\t";
        }

        trainFile<<((itr->second).SL.coach.size())<<"\t";
        if((itr->second).SL.coach.size() == 0)
        {
            trainFile<<"0\t ";
        }
        else
        {
            trainFile<<((itr->second).SL.coach)["S-1"].num_seats<<"\t";
            //cout<<(itr->second).SL.coach.size()<<endl;
            for(int i=1; i<=((itr->second).SL.coach.size()); ++i)
            {

                out.str(std::string());
                out<<i;

                for(int j = 0; j < (((itr->second).SL.coach)["S-"+out.str()].num_available).size(); ++j)
                    trainFile<<(((itr->second).SL.coach)["S-"+out.str()].num_available)[j]<<" ";

                //cout<<i<<endl;
                //cout<<"After while 2"<<endl;
            }
            //cout<<"After while 3"<<endl;

            //trainFile<<"\t";
        }
        trainFile<<"\n";
        itr++;
    }
    trainFile.close();
    cout<<"Successfully updated Database"<<endl;
}


// Function For Printing a ticket of a given PNR
string printTicket(int PNR)
{
    Ticket ticket;

    map<int,string> seatType;

    seatType[1] = "LB";
    seatType[2] = "MB";
    seatType[3] = "UB";
    seatType[4] = "LB";
    seatType[5] = "MB";
    seatType[6] = "UB";
    seatType[7] = "SL";
    seatType[0] = "SU";

    map<int,string> seatType_Rajdhani;

    seatType_Rajdhani[1] = "LB";
    seatType_Rajdhani[2] = "UB";
    seatType_Rajdhani[3] = "LB";
    seatType_Rajdhani[4] = "UB";
    seatType_Rajdhani[5] = "SL";
    seatType_Rajdhani[0] = "SU";

    map<int,Ticket>::iterator itr;
    stringstream out;
    out.str(std::string());
    itr = tickets.find(PNR);
    if(itr == tickets.end())
    {
        out<<"Invalid PNR...Try Again"<<endl;
    }
    else{
        ticket = tickets[PNR];
        out<<"PNR: "<<PNR<<endl;
        out<<"Train Number: "<<ticket.trainNo<<"\tTrain Name: "<<trains[ticket.trainNo].name<<endl;
        out<<"Class: "<<ticket.coachType<<endl;
        out<<"Number Of Berths: "<<ticket.num_berths<<endl;
        out<<"Seat Information: "<<endl;
        multimap<string,seatInfo>::iterator itr1;
        itr1 = ticket.seat.begin();
        while(itr1 != ticket.seat.end())
        {
        	if(ticket.trainNo == "12301")
            	out<<(itr1->first)<<"\t"<<(itr1->second).seatNo<<"\t"<<seatType_Rajdhani[((itr1->second).seatNo)%6]<<"\t"<<(itr1->second).age<<endl;
            else out<<(itr1->first)<<"\t"<<(itr1->second).seatNo<<"\t"<<seatType[((itr1->second).seatNo)%8]<<"\t"<<(itr1->second).age<<endl;
            itr1++;
        }
        out<<endl;
    }
    return out.str();
}

// Function for Processing a Booking Request and generating the Ticket
int processRequest(string passId,Ticket ticket)
{
    map<string,int> pref;
    map<string,vector<int> > pref_age;
    pref_age.clear();
    pref.clear();

    pref["LB"] = 0;
    pref["MB"] = 0;
    pref["UB"] = 0;
    pref["SL"] = 0;
    pref["SU"] = 0;

    pref_age["LB"].clear();
    pref_age["MB"].clear();
    pref_age["UB"].clear();
    pref_age["SL"].clear();
    pref_age["SU"].clear();


    if(ticket.preferences[0] != "NA")
    {
        for(int i = 0; i< ticket.preferences.size(); ++i)
        {
            pref[ticket.preferences[i]] ++;
            pref_age[ticket.preferences[i]].push_back(ticket.ages[i]);
        }
    }

    //cout<<pref["LB"]<<"\t"<<pref["MB"]<<"\t"<<pref["UB"]<<"\t"<<pref["SL"]<<"\t"<<pref["SU"]<<endl;

    map<string,Train>::iterator itr;
    itr = trains.find(ticket.trainNo);
    if(itr == trains.end())
    {
        return 0;
    }
    else
    {
        Train t = trains[ticket.trainNo];

        // Check the type of coach
        if(ticket.coachType == "AC")
        {
            int tot_available=0;
            int tot_req = ticket.num_berths;
            map<string,Coach>::iterator itr1;
            itr1 = t.AC.coach.begin();

            // Count Number of Seats Available
            while(itr1 != t.AC.coach.end())
            {
                for(int j = 0; j< (itr1->second).num_available.size(); j++)
                    tot_available += (itr1->second).num_available[j];

                itr1++;
            }

            // If Seats are available then go for booking
            if(tot_available>=ticket.num_berths)
            {
                itr1 = t.AC.coach.begin();
                // Loop through all the coaches
                while(itr1 != t.AC.coach.end())
                {

                	// Check if LB is prefered and is available in this coach
                    if(pref["LB"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%8 == 1 || (j+1)%8 == 4)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;
                                    pref["LB"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["LB"][pref_age["LB"].size()-1];
                                    pref_age["LB"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["LB"] == 0) break;
                                }
                            }
                        }

                    }


                    // Check if MB is prefered and is available in this coach
                    if(pref["MB"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%8 == 2 || (j+1)%8 == 5)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;
                                    pref["MB"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["MB"][pref_age["MB"].size()-1];
                                    pref_age["MB"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["MB"] == 0) break;
                                }
                            }
                        }

                    }

                    // Check if UB is prefered and is available in this coach
                    if(pref["UB"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%8 == 3 || (j+1)%8 == 6)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;
                                    pref["UB"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["UB"][pref_age["UB"].size()-1];
                                    pref_age["UB"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["UB"] == 0) break;
                                }
                            }
                        }

                    }

                    // Check if SL is prefered and is available in this coach
                    if(pref["SL"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%8 == 7)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;
                                    pref["SL"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["SL"][pref_age["SL"].size()-1];
                                    pref_age["SL"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["SL"] == 0) break;
                                }
                            }
                        }

                    }

                    // Check if SU is prefered and is available in this coach
                    if(pref["SU"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%8 == 0)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;
                                    pref["SU"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["SU"][pref_age["SU"].size()-1];
                                    pref_age["SU"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["SU"] == 0) break;
                                }
                            }
                        }

                    }

                    // preferences done

                    // If No more berths are needed break
                    if(tot_req == 0) break;

                    // Else look for seats in current Coach
                    for(int j = 0; j< (itr1->second).num_seats; j++)
                    {
                        if(tot_req == 0) break;
                        else if((itr1->second).num_available[j] == 1)
                        {
                            trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;

                            seatInfo si;
                            si.seatNo = j+1;
                            if(pref["LB"]!=0)
                            {
                            	pref["LB"]--;
                            	si.age = pref_age["LB"][pref_age["LB"].size()-1];
                                pref_age["LB"].pop_back();
                            }
                            else if(pref["MB"]!=0)
                           	{
                           		pref["MB"]--;
                            	si.age = pref_age["MB"][pref_age["MB"].size()-1];
                                pref_age["MB"].pop_back();
                            }
                            else if(pref["UB"]!=0)
                            {
                            	pref["UB"]--;
                            	si.age = pref_age["UB"][pref_age["UB"].size()-1];
                                pref_age["UB"].pop_back();
                            }
                            else if(pref["SL"]!=0)
                            {
                            	pref["SL"]--;
                            	si.age = pref_age["SL"][pref_age["SL"].size()-1];
                                pref_age["SL"].pop_back();
                            }
                            else if(pref["SU"]!=0)
                            {
                            	pref["SU"]--;
                            	si.age = pref_age["SU"][pref_age["SU"].size()-1];
                                pref_age["SU"].pop_back();
                            }
                            
                            else{
                            	si.age = ticket.ages[(ticket.ages.size())-1];
                            	ticket.ages.pop_back();
                            }

                            ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));
                            tot_req--;
                        }
                    }

                    itr1++;
                }

            }
            else
            {
                return 1;
            }
        }

        // Repeat the same thing for Sleeper
        else if(ticket.coachType == "SL" || ticket.coachType == "Sleeper")
        {

            int tot_available=0;
            int tot_req = ticket.num_berths;
            map<string,Coach>::iterator itr1;
            itr1 = t.SL.coach.begin();
            while(itr1 != t.SL.coach.end())
            {
                for(int j = 0; j< (itr1->second).num_available.size(); j++)
                    tot_available += (itr1->second).num_available[j];

                itr1++;
            }
            if(tot_available>=ticket.num_berths)
            {
                itr1 = t.SL.coach.begin();
                while(itr1 != t.SL.coach.end())
                {
                    if(pref["LB"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%8 == 1 || (j+1)%8 == 4)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].SL.coach[(itr1->first)].num_available[j] = 0;
                                    pref["LB"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["LB"][pref_age["LB"].size()-1];
                                    pref_age["LB"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["LB"] == 0) break;
                                }
                            }
                        }

                    }

                    if(pref["MB"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%8 == 2 || (j+1)%8 == 5)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].SL.coach[(itr1->first)].num_available[j] = 0;
                                    pref["MB"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["MB"][pref_age["MB"].size()-1];
                                    pref_age["MB"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["MB"] == 0) break;
                                }
                            }
                        }

                    }


                    if(pref["UB"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%8 == 3 || (j+1)%8 == 6)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].SL.coach[(itr1->first)].num_available[j] = 0;
                                    pref["UB"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["UB"][pref_age["UB"].size()-1];
                                    pref_age["UB"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["UB"] == 0) break;
                                }
                            }
                        }

                    }


                    if(pref["SL"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%8 == 7)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].SL.coach[(itr1->first)].num_available[j] = 0;
                                    pref["SL"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["SL"][pref_age["SL"].size()-1];
                                    pref_age["SL"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["SL"] == 0) break;
                                }
                            }
                        }

                    }


                    if(pref["SU"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%8 == 0)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].SL.coach[(itr1->first)].num_available[j] = 0;
                                    pref["SU"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["SU"][pref_age["SU"].size()-1];
                                    pref_age["SU"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["SU"] == 0) break;
                                }
                            }
                        }

                    }

                    // preferences done;

                    if(tot_req == 0) break;

                    for(int j = 0; j< (itr1->second).num_seats; j++)
                    {
                        if(tot_req == 0) break;
                        else if((itr1->second).num_available[j] == 1)
                        {
                            seatInfo si;
                            si.seatNo = j+1;
                            if(pref["LB"]!=0)
                            {
                            	pref["LB"]--;
                            	si.age = pref_age["LB"][pref_age["LB"].size()-1];
                                pref_age["LB"].pop_back();
                            }
                            else if(pref["MB"]!=0)
                           	{
                           		pref["MB"]--;
                            	si.age = pref_age["MB"][pref_age["MB"].size()-1];
                                pref_age["MB"].pop_back();
                            }
                            else if(pref["UB"]!=0)
                            {
                            	pref["UB"]--;
                            	si.age = pref_age["UB"][pref_age["UB"].size()-1];
                                pref_age["UB"].pop_back();
                            }
                            else if(pref["SL"]!=0)
                            {
                            	pref["SL"]--;
                            	si.age = pref_age["SL"][pref_age["SL"].size()-1];
                                pref_age["SL"].pop_back();
                            }
                            else if(pref["SU"]!=0)
                            {
                            	pref["SU"]--;
                            	si.age = pref_age["SU"][pref_age["SU"].size()-1];
                                pref_age["SU"].pop_back();
                            }
                            
                            else{
                            	si.age = ticket.ages[(ticket.ages.size())-1];
                            	ticket.ages.pop_back();
                            }

                            ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));
                            tot_req--;
                        }
                    }

                    itr1++;
                }

            }
            else
            {
                return 1;
            }

        }
        else return 1;
    }

    PNR_Count++;
    tickets.insert(pair<int,Ticket>(PNR_Count,ticket));
    return PNR_Count;
}

// Process request if the train is Rajdhani(Different seat format)
int processRequest_Rajdhani(string passId,Ticket ticket)
{
    map<string,int> pref;
    map<string,vector<int> > pref_age;
    pref_age.clear();
    pref.clear();

    pref["LB"] = 0;
    pref["UB"] = 0;
    pref["SL"] = 0;
    pref["SU"] = 0;

    pref_age["LB"].clear();
    pref_age["UB"].clear();
    pref_age["SL"].clear();
    pref_age["SU"].clear();


    if(ticket.preferences[0] != "NA")
    {
        for(int i = 0; i< ticket.preferences.size(); ++i)
        {
            pref[ticket.preferences[i]] ++;
            pref_age[ticket.preferences[i]].push_back(ticket.ages[i]);
        }
    }

   // cout<<pref["LB"]<<"\t"<<pref["UB"]<<"\t"<<pref["SL"]<<"\t"<<pref["SU"]<<endl;

    map<string,Train>::iterator itr;
    itr = trains.find(ticket.trainNo);
    if(itr == trains.end())
    {
        return 0;
    }
    else
    {
        Train t = trains[ticket.trainNo];
        if(ticket.coachType == "AC")
        {
            int tot_available=0;
            int tot_req = ticket.num_berths;
            map<string,Coach>::iterator itr1;
            itr1 = t.AC.coach.begin();
            while(itr1 != t.AC.coach.end())
            {
                for(int j = 0; j< (itr1->second).num_available.size(); j++)
                    tot_available += (itr1->second).num_available[j];

                itr1++;
            }
            if(tot_available>=ticket.num_berths)
            {
                itr1 = t.AC.coach.begin();
                while(itr1 != t.AC.coach.end())
                {
                    if(pref["LB"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%6 == 1 || (j+1)%6 == 3)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;
                                    pref["LB"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["LB"][pref_age["LB"].size()-1];
                                    pref_age["LB"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["LB"] == 0) break;
                                }
                            }
                        }

                    }

                 

                    if(pref["UB"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%6 == 2 || (j+1)%6 == 4)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;
                                    pref["UB"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["UB"][pref_age["UB"].size()-1];
                                    pref_age["UB"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["UB"] == 0) break;
                                }
                            }
                        }

                    }


                    if(pref["SL"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%6 == 5)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;
                                    pref["SL"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["SL"][pref_age["SL"].size()-1];
                                    pref_age["SL"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["SL"] == 0) break;
                                }
                            }
                        }

                    }


                    if(pref["SU"] != 0)
                    {
                        for(int j = 0; j< (itr1->second).num_seats; j++)
                        {
                            if((j+1)%6 == 0)
                            {
                                if((itr1->second).num_available[j] == 1)
                                {
                                    tot_req--;
                                    trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;
                                    pref["SU"]--;
                                    seatInfo si;
                                    si.seatNo = (j+1);
                                    si.age = pref_age["SU"][pref_age["SU"].size()-1];
                                    pref_age["SU"].pop_back();
                                    ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));

                                    if(pref["SU"] == 0) break;
                                }
                            }
                        }

                    }

                    // preferences done;

                    if(tot_req == 0) break;

                    for(int j = 0; j< (itr1->second).num_seats; j++)
                    {
                        if(tot_req == 0) break;
                        else if((itr1->second).num_available[j] == 1)
                        {
                            trains[ticket.trainNo].AC.coach[(itr1->first)].num_available[j] = 0;

                            seatInfo si;
                            si.seatNo = j+1;
                            if(pref["LB"]!=0)
                            {
                            	pref["LB"]--;
                            	si.age = pref_age["LB"][pref_age["LB"].size()-1];
                                pref_age["LB"].pop_back();
                            }
                            
                            else if(pref["UB"]!=0)
                            {
                            	pref["UB"]--;
                            	si.age = pref_age["UB"][pref_age["UB"].size()-1];
                                pref_age["UB"].pop_back();
                            }
                            else if(pref["SL"]!=0)
                            {
                            	pref["SL"]--;
                            	si.age = pref_age["SL"][pref_age["SL"].size()-1];
                                pref_age["SL"].pop_back();
                            }
                            else if(pref["SU"]!=0)
                            {
                            	pref["SU"]--;
                            	si.age = pref_age["SU"][pref_age["SU"].size()-1];
                                pref_age["SU"].pop_back();
                            }
                            
                            else{
                            	si.age = ticket.ages[(ticket.ages.size())-1];
                            	ticket.ages.pop_back();
                            }

                            ticket.seat.insert(pair<string,seatInfo>(itr1->first, si));
                            tot_req--;
                        }
                    }

                    itr1++;
                }

            }
            else
            {
                return 1;
            }
        }
        else return 1;

    }

    PNR_Count++;
    tickets.insert(pair<int,Ticket>(PNR_Count,ticket));
    return PNR_Count;
}


// Process the requests in queue
void processQueue(Priority priority,Ticket ticket){
	string result;
	int res;
	result.clear();

	if(ticket.trainNo == "12301")
		res = processRequest_Rajdhani(priority.passId,ticket);
	else res = processRequest(priority.passId,ticket);
	string passId = priority.passId;
    cout<<res<<endl;
    
    if(res == 0)
    {
        result = "Request "+passId+":\nInvalid Train Number...Try Again\n";
    }
    else if(res == 1)
    {
        result = "Request "+passId+":\nSorry...Seats Not Available\n";   
    }

    else{
        result = "Request "+passId+":\n"+printTicket(res);
    }
    cout<<result<<endl;
    updateFile();
    send_msg(priority.agentFD,result);
    string recmsg = receive_msg(priority.agentFD,MESSAGE_LENGTH);
}


// Parse the request given by agent and put it in a queue
void bookTicket(string form,int agentFD)
{
    Ticket ticket;
    string passId,in,out,result;
    Priority priority;
    int n = form.length();
    cout<<n<<endl;
    int i = 0,f = 0;
    while(i<n)
    {
        while(form[i]!='\n')
        {
            if(i>=n) break;
            while(form[i]!='\t')
            {
                if(form[i] == '\n' || i>=n) break;
                if(form[i]!=' ')
                    in += form[i];
                i++;
            }
            i++;

            switch(f)
            {
            case 0:
                priority.no_of_passengers = 0;
                priority.no_of_seniors = 0;
                priority.timestamp = 0;
                priority.agentFD = agentFD;
                priority.passId.clear();
                priority.timestamp = atol(in.c_str());
                in.clear();
                f = 1;
                break;
            case 1:
            	priority.passId.clear();
            	priority.passId = in;
                passId = in;
                in.clear();
                f = 2;
                break;
            case 2:
                ticket.trainNo = in;
                in.clear();
                
                f = 3;
                break;
            case 3:
                ticket.coachType = in;
                in.clear();
                f = 4;
                break;
            case 4:
            	priority.no_of_passengers = 0;
                ticket.num_berths = atoi(in.c_str());
                priority.no_of_passengers = ticket.num_berths;
                in.clear();
                f = 5;
                break;
            case 5:
                out.clear();
                cout<<in<<endl;
                ticket.preferences.clear();
                for(int j = 0; j<in.length(); j++)
                {
                    if(in[j] == '-')
                    {
                        ticket.preferences.push_back(out);
                        out.clear();
                    }
                    else out += in[j];
                }
                ticket.preferences.push_back(out);
                in.clear();
                out.clear();
                f = 6;
                break;
            case 6:
            	priority.no_of_seniors = 0;
                out.clear();
                ticket.ages.clear();

                for(int j=0; j<in.length(); j++)
                {
                    if(in[j]=='-')
                    {
                        ticket.ages.push_back(atoi(out.c_str()));
                        if(atoi(out.c_str())>=60){
                        	priority.no_of_seniors++;
                        }
                        out.clear();
                    }
                    else out += in[j];
                }
                ticket.ages.push_back(atoi(out.c_str()));
                if(atoi(out.c_str())>=60){
                  	priority.no_of_seniors++;
                }
                out.clear();

                booking_queue.insert(pair<Priority,Ticket>(priority,ticket));
                in.clear();
                f = 0;
                break;
            }

        }
        i++;
    }
}

// Function for communicating with agent
int agent(int agentFD)
{
    string recmsg,recmsg1,sendmsg,parsed;

    recmsg1.clear();
    recmsg1 = receive_msg(agentFD,MESSAGE_LENGTH);
    int i  = 0;

    recmsg.clear();
    parsed.clear();

    while(recmsg1[i]!='\t' && i<recmsg1.length())
    {
        parsed += recmsg1[i];
        i++;
    }

    i++;
    while(i<recmsg1.length())
    {
        recmsg += recmsg1[i];
        i++;
    }


    // Request for Train Details
    if(parsed == "1")
    {
        sendmsg.clear();
        sendmsg = "Enter Train Number: ";
        send_msg(agentFD,sendmsg);
    }

    // Send Train Details
    else if(parsed == "5")
    {
        map<string,Train>::iterator itr;
        itr = trains.find(recmsg);
        if(itr!=trains.end())
        {
            stringstream out,out1;
            out.str(std::string());
            //out<<setiosflags(out::left);
            out<<"Train Details"<<endl;
            out<<"----------------------------------------------------------------------------------------------------"<<endl;
            out<<std::left<<setw(12)<<"Train No.";
            out<<std::left<<setw(25)<<"Train Name";
            out<<std::left<<setw(30)<<"Total[Booked/Available](AC)";
            out<<std::left<<setw(30)<<"Total[Booked/Available](SL)"<<endl;
            int num_berths = 0;
            int num_avail = 0;
            int num_book = 0;
            out<<std::left<<setw(12)<<(itr->first);
            out<<std::left<<setw(25)<<(itr->second).name;
            map<string,Coach>::iterator itr1;
            itr1 = ((itr->second).AC.coach).begin();
            while(itr1!=((itr->second).AC.coach).end())
            {
                num_berths += ((itr1->second)).num_seats;
                for(int j = 0; j< (((itr1->second)).num_available.size()); ++j)
                {
                    if((((itr1->second)).num_available[j] == 1))
                        num_avail++;

                    else num_book++;

                }

                itr1++;
            }
            out1.str(std::string());
            out1<<num_berths<<"["<<num_book<<"/"<<num_avail<<"]";
            out<<std::left<<setw(30)<<out1.str();

            num_berths = 0;
            num_avail = 0;
            num_book = 0;
            itr1 = ((itr->second).SL.coach).begin();
            while(itr1!=((itr->second).SL.coach).end())
            {
                num_berths += ((itr1->second)).num_seats;
                for(int j = 0; j< (((itr1->second)).num_available.size()); ++j)
                {
                    if((((itr1->second)).num_available[j] == 1))
                        num_avail++;

                    else num_book++;

                }

                itr1++;
            }
            out1.str(std::string());
            out1<<num_berths<<"["<<num_book<<"/"<<num_avail<<"]";
            out<<std::left<<setw(30)<<out1.str();

            out<<endl;
            out<<"----------------------------------------------------------------------------------------------------"<<endl;
            out<<"End Of Train Details"<<endl;
            sendmsg.clear();
            sendmsg = out.str();
            send_msg(agentFD,sendmsg);
            receive_msg(agentFD,MESSAGE_LENGTH);
            sendmsg = "Do you want to continue(y/n): ";
            send_msg(agentFD,sendmsg);
        }
        else
        {
            sendmsg.clear();
            sendmsg = "Invalid Train Number...Try Again\n";
            send_msg(agentFD,sendmsg);
            receive_msg(agentFD,MESSAGE_LENGTH);
            sendmsg = "Do you want to continue(y/n): ";
            send_msg(agentFD,sendmsg);
        }

    }

    // Request For Booking
    else if(parsed == "2")
    {
        sendmsg = "Enter path of .csv file containing booking details: ";
        send_msg(agentFD,sendmsg);
    }

    // Book the Tickets
    else if(parsed == "4")
    {
        bookTicket(recmsg,agentFD);
    }

    else if(parsed == "3")
    {
        return -1;
    }
    
    // Output of continue message
    if(parsed == "6"){
        if(recmsg == "y"){
            sendmsg = "Welcome to INDIAN RAILWAY BOOKING SERVICE\n";
            sendmsg += "Enter 1 to get Status of a Train\n";
            sendmsg += "Enter 2 to book tickets\n";
            sendmsg += "Enter 3 to exit\n";
            send_msg(agentFD,sendmsg);
        }
        else if(recmsg == "n") return -1;
    }

    return 0;
}


// Load Train Data From File at the start of server
void loadFile()
{
    ifstream infile(path.c_str());
    stringstream strStream;
    strStream << infile.rdbuf();//read the file
    string text = strStream.str();//str holds the content of the file
    infile.close();
    int i = 0,f = 0,j,k,ii;
    int n = text.length();
    string in;
    stringstream out;
    string trainNum,name,num_AC_booked,num_AC_avail,num_SL_booked,num_SL_avail,book,avail;
    int num_AC_coaches,num_AC_seats,num_SL_coaches,num_SL_seats;
    Train train;
    Coach coach;
    while(i<n)
    {
        while(text[i]!='\n')
        {
            if(i==n) break;
            while(text[i]!='\t')
            {
                if(text[i]=='\n') break;
                in += text[i];
                i++;
            }
            switch(f)
            {
            case 0:
                trainNum = in;
                f = 1;
                in.clear();
                break;

            case 1:
                name = in;
                f = 2;
                in.clear();
                break;
            case 2:
                num_AC_coaches = atoi(in.c_str());
                f = 3;
                in.clear();
                break;
            case 3:
                num_AC_seats = atoi(in.c_str());
                f = 4;
                in.clear();
                break;
            case 4:
                num_AC_avail = in;
                f = 5;
                in.clear();
                break;

            case 5:
                num_SL_coaches = atoi(in.c_str());
                f = 6;
                in.clear();
                break;
            case 6:
                num_SL_seats = atoi(in.c_str());
                f = 7;
                in.clear();
                break;
            case 7:
                num_SL_avail = in;
                train.name = name;
                train.AC.coach.clear();
                train.SL.coach.clear();

                j = 0;
                k = 0;
                for(ii=1; ii<=num_AC_coaches; ++ii)
                {
                    coach.num_seats = num_AC_seats;
                    out.str(std::string());
                    out<<ii;

                    coach.num_available.clear();
                    for(int iii=0; iii<num_AC_seats; ++iii)
                    {
                        avail.clear();
                        while(num_AC_avail[k]!=' ')
                        {
                            avail += num_AC_avail[k];
                            k++;
                        }
                        coach.num_available.push_back(atoi(avail.c_str()));
                        k++;
                    }
                    train.AC.coach.insert(pair<string,Coach>(("A-"+out.str()),coach));
                }

                j = 0;
                k = 0;

                for(ii=1; ii<=num_SL_coaches; ++ii)
                {
                    coach.num_seats = num_SL_seats;

                    out.str(std::string());
                    out<<ii;
                    coach.num_available.clear();
                    for(int iii=0; iii<num_SL_seats; ++iii)
                    {
                        avail.clear();
                        while(num_SL_avail[k]!=' ')
                        {
                            avail += num_SL_avail[k];
                            k++;
                        }
                        coach.num_available.push_back(atoi(avail.c_str()));
                        k++;
                    }

                    train.SL.coach.insert(pair<string,Coach>(("S-"+out.str()),coach));
                }


                trains.insert(pair<string,Train>(trainNum,train));

                f = 0;
                in.clear();
                break;

            }
            i++;
        }
        i++;
    }
}

// Insert a new Train into database
void new_train()
{
    Train train;
    string trainNum;
    stringstream out;
    int num_coach;
    int num_births,i;

    (trainNum).clear();
    cout<<"Enter Train Number: ";
    cin>>(trainNum);

    (train.name).clear();
    cout<<"Enter the Name of the Train: ";
    cin>>(train.name);

    cout<<"Enter number of AC coaches: ";
    cin>>(num_coach);


    cout<<"Enter number of births per AC Coach: ";
    cin>>(num_births);

    for(i=1; i<=num_coach; ++i)
    {
        Coach coach;
        coach.num_seats = num_births;
        for(int j = 0; j<num_births; ++j)
            coach.num_available.push_back(1);

        out.str(std::string());
        out<<i;
        train.AC.coach.insert(pair<string,Coach>("A-"+out.str(),coach));
    }


    cout<<"Enter number of Sleeper coaches: ";
    cin>>(num_coach);

    cout<<"Enter number of births per Sleeper Coach: ";
    cin>>(num_births);

    for(i=1; i<=num_coach; ++i)
    {
        Coach coach;
        coach.num_seats = num_births;
        for(int j = 0; j<num_births; ++j)
            coach.num_available.push_back(1);

        out.str(std::string());
        out<<i;
        train.SL.coach.insert(pair<string,Coach>("S-"+out.str(),coach));
    }

    trains.insert(pair<string,Train>(trainNum,train));

    cout<<"Successfully Inserted New Train: "<<trainNum<<endl;

}

// Print the status of all trains
void printStatus()
{
    map<string,Train>::iterator itr;
    itr = trains.begin();
    cout<<setiosflags(ios::left);
    cout<<"LIST OF ALL TRAINS"<<endl;
    cout<<"------------------------------------------------------------------------------------------------------------------"<<endl;

    cout<<setw(12)<<"Train No.";
    cout<<setw(25)<<"Train Name";
    cout<<setw(30)<<"Total[Booked/Available](AC)";
    cout<<setw(30)<<"Total[Booked/Available](SL)"<<endl<<endl;
    
    stringstream out;
    while(itr!=trains.end())
    {
        int num_berths = 0;
        int num_avail = 0;
        int num_book = 0;
        cout<<setw(12)<<(itr->first);
        cout<<setw(25)<<(itr->second).name;
        map<string,Coach>::iterator itr1;
        itr1 = ((itr->second).AC.coach).begin();
        while(itr1!=((itr->second).AC.coach).end())
        {
            num_berths += ((itr1->second)).num_seats;

            for(int j = 0; j< (((itr1->second)).num_available.size()); ++j)
            {
                if((((itr1->second)).num_available[j] == 1))
                    num_avail++;

                else num_book++;

            }

            itr1++;
        }
        out.str(std::string());
        out<<num_berths<<"["<<num_book<<"/"<<num_avail<<"]";
        cout<<setw(30)<<out.str();

        num_berths = 0;
        num_avail = 0;
        num_book = 0;
        itr1 = ((itr->second).SL.coach).begin();
        while(itr1!=((itr->second).SL.coach).end())
        {
            num_berths += ((itr1->second)).num_seats;
            for(int j = 0; j< (((itr1->second)).num_available.size()); ++j)
            {
                if((((itr1->second)).num_available[j] == 1))
                    num_avail++;

                else num_book++;

            }

            itr1++;
        }
        out.str(std::string());
        out<<num_berths<<"["<<num_book<<"/"<<num_avail<<"]";
        cout<<setw(30)<<out.str();

        cout<<endl;
        itr++;
    }
    cout<<endl<<"------------------------------------------------------------------------------------------------------------"<<endl;
    cout<<"END OF LIST"<<endl<<endl;
}

// Query the server
void *query(void *args)
{
    string in;
    while(1)
    {
    	cout<<"Press 0 to clear the terminal"<<endl;
        cout<<"Press 1 to display current Booking Status of all the trains"<<endl;
        cout<<"Press 2 for creating a new train"<<endl;
        cout<<"Press 3 to save data and quit"<<endl;
        cout<<"Press 4 for quiting without saving"<<endl;
        cin>>in;
        if(in == "0")
        {
        	printf("\033[H\033[J");
        	continue;
        }

        else if(in == "1")
        {
            printStatus();
        }
        else if(in=="2")
        {
            new_train();
            updateFile();
        }
        else if(in == "3")
        {
            updateFile();
            exit(1);
        }
        else if(in == "4")
        {
            exit(1);
        }
        else
        {
            cout<<"Invalid Input"<<endl;
        }
    }
}

int main(int argc, char * argv[])
{
	printf("\033[H\033[J");
    loadFile();
    cout<<"Loading Done"<<endl;
    int servfd = socket(AF_INET,SOCK_STREAM,0);
    PORT_NO = atoi(argv[1]);
    printf("%d\n",servfd);
    int pid;
    liveThreads = 0;
    char s[3000];
    //system ("fuser -k 5000/tcp");
    if(servfd < 0)printf("Server socket could not be connected\n");
    char  msg[MESSAGE_LENGTH];
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(PORT_NO);

    while(bind(servfd, (struct sockaddr *)&saddr,sizeof(saddr)) <0 )
    {
        printf("Bind Unsuccessful\n");
        cin>>PORT_NO;
        saddr.sin_port = htons(PORT_NO);
    }
    if(listen(servfd,NO_OF_CLIENTS) <0 )printf("Listen unsuccessful\n");


    fd_set active_sock_fd_set, read_fd_set;


    pthread_t td;

    try{
    pthread_create(&td,NULL,&query,NULL);

    }catch(const std::exception& e)
    {
        cout<<e.what()<<endl;
    }

    // Initialize the set of active sockets

    FD_ZERO(&active_sock_fd_set);
    FD_SET(servfd, &active_sock_fd_set);

    while(1)
    {

        read_fd_set = active_sock_fd_set;

        if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Service all the sockets with input pending

        for(int i=0;i<FD_SETSIZE;++i)
        {
            if(FD_ISSET(i, &read_fd_set))
            {
                if(i==servfd)
                {
                    int connfd = accept(servfd, (struct sockaddr*)NULL, NULL);
                    if(connfd<0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    cout<<"Starting new Connection..."<<connfd<<endl;
                    string sendmsg = "Welcome to INDIAN RAILWAY BOOKING SERVICE\n";
                    sendmsg += "Enter 1 to get Status of a Train\n";
                    sendmsg += "Enter 2 to book tickets\n";
                    sendmsg += "Enter 3 to exit\n";
                    send_msg(connfd,sendmsg);

                    FD_SET(connfd,&active_sock_fd_set);
                }
                else{
                    // Data arrived from already connected socket
                    if(agent(i) == -1)
                    {
                        cout<<"Closing connection "<<i<<endl;
                        close(i);
                        FD_CLR (i, &active_sock_fd_set);
                    }

                }
            }
        }

        multimap<Priority,Ticket,Comp>::iterator itr;

        set<int> FDs;

        FDs.clear();

        itr = booking_queue.begin();

        while(itr!=booking_queue.end())
        {
        	processQueue(itr->first, itr->second);
        	FDs.insert((itr->first).agentFD);
        	itr++;
        }

        booking_queue.clear();

        set<int>::iterator itrs = FDs.begin();

        while(itrs != FDs.end())
        {
        	string sendmsg = "Do you want to continue(y/n): ";
        	send_msg(*itrs,sendmsg);
        	itrs++;
        }

        FDs.clear();

        sleep(1);
    }
    return 0;
}
