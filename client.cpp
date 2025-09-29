/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Caleb Santos
	UIN: 134008179
	Date: 09/18/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;


int main (int argc, char *argv[]) {
	//cout << argv[0];
	
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	// default value for buffer capacity
	//int m = MAX_MESSAGE;
	bool new_chan = false;
	bool bulk = false; // if only -p is passed
	vector<FIFORequestChannel*> channels;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:c")) != -1) {
		cout << "While loop";
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			//case 'm':
				//m = atoi (optarg);
				//break;
			case 'c':
				new_chan = true;
				break;
		}
	}

	if (p != -1 && t == -1.0 && e == -1 && filename.empty()) {
		bulk = true;
	}

	// Kick off the server, give arguments
	char* servArgs[] = {(char*) "./server", (char*) "-m", (char*) "256", nullptr};

	// Creating pipe
	/*int fds[2];
	if (pipe(fds) == -1) {
		cerr << "pipe failed\n";
		return 1;
	}*/

	// Creates child
	pid_t serverChild = fork();
	
	if(serverChild == -1) {
		cerr << "fork failed\n";
        return 1;
	}
	if (serverChild == 0) {
		execvp(servArgs[0], servArgs);
		cerr << "child failed";
		return 1;
	} /*else {
		int s;
		waitpid(serverChild, &s, 0);
	}*/

	FIFORequestChannel* control_chan = nullptr;
	const int MAX_TRIES = 50;
	int tries = 0;

	while (tries < MAX_TRIES) {
		try {
			control_chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
			break;
		} catch (...) {
			this_thread::sleep_for(chrono::milliseconds(100));
			tries++;
		}
	}

	if (!control_chan) {
		cerr << "Couldn't open control channel";
		return 1;
	}
    //FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(control_chan);

	FIFORequestChannel* data_chan = nullptr;
	if(new_chan) {
		// send newchannel request to the server
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		control_chan->cwrite(&nc, sizeof(MESSAGE_TYPE));
		// create a variable to hold the name
		char newname[256];
		// cread the response from the server
		memset(newname, 0, sizeof(newname));
		control_chan->cread(newname, sizeof(newname));
		// call the FIFORequestChannel constructor with the name from the server
		// push the new channel into the vector
		try {
			data_chan = new FIFORequestChannel(newname, FIFORequestChannel::CLIENT_SIDE);
			channels.push_back(data_chan);
		} catch (...) {
			cerr << "Failed to create channel";
			data_chan = nullptr;
		}
	}

	FIFORequestChannel chan = *(data_chan ? data_chan : control_chan);

	// Single Datapoint, only run p,t,e != -1

	// example data point request
	if (p != -1 && t != -1.0 && e != -1) {
		char buf[MAX_MESSAGE]; // 256
    	datamsg x(p, t, e); // change from hardcoding to user's values
		memset(buf, 0, sizeof(buf));
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}

	// Else, if p != -1, request 1000 datapoints.
	// Loop over 1st 1000 lines
	// Send request for ecg 1
	// Send request for ecg 2
	// Write line to received
	
	if (bulk) {
		mkdir("received", 0755);
		// Create output file
		ofstream ofs("received/x1.csv");
		if (!ofs) {
			cerr << "Couldn't open x1.csv for writing\n";
		} else {
			// We will write: time, ecg1, ecg2
			// NOTE: the sampling interval (delta t) depends on your dataset. Many labs use 0.004.
			const double dt = 0.004; // change if your dataset uses a different timestep
			for (int i = 0; i < 1000; ++i) {
				double time_i = i * dt;

				// request ecg1
				datamsg d1(p, time_i, 1);
				char b1[MAX_MESSAGE];
				memset(b1, 0, sizeof(b1));
				memcpy(b1, &d1, sizeof(datamsg));
				chan.cwrite(b1, sizeof(datamsg));
				double v1;
				chan.cread(&v1, sizeof(double));

				// request ecg2
				datamsg d2(p, time_i, 2);
				char b2[MAX_MESSAGE];
				memset(b2, 0, sizeof(b2));
				memcpy(b2, &d2, sizeof(datamsg));
				chan.cwrite(b2, sizeof(datamsg));
				double v2;
				chan.cread(&v2, sizeof(double));

				ofs << time_i << "," << v1 << "," << v2 << "\n";
			}
			ofs.close();
			cout << "Wrote first 1000 datapoints for patient " << p << " into x1.csv\n";
			// You can diff x1.csv with the provided benchmark file outside this program.
		}
	}

    // sending a non-sense message, you need to change this
	if (!filename.empty()) {
		filemsg fm(0, 0);
		//string fname = "teslkansdlkjflasjdf.dat";
		
		int len = sizeof(filemsg) + (filename.size() + 1);
		//char* buf2 = new char[len];
		vector<char> reqbuf(len, 0);
		//memset(buf2, 0, len);
		//memcpy(buf2, &fm, sizeof(filemsg));
		//strcpy(buf2 + sizeof(filemsg), filename.c_str());
		memcpy(reqbuf.data(), &fm, sizeof(filemsg));
		memcpy(reqbuf.data() + sizeof(filemsg), filename.c_str(), filename.size() + 1);
		chan.cwrite(reqbuf.data(), len);  // I want the file length;
		int64_t filesize = 0;
		chan.cread(&filesize, sizeof(int64_t));
		//delete[] buf2;

		if (filesize < 0) {
			cerr << "File not found\n";
		} else {
			cout << "File '" << filename << "' reported size: " << filesize << " bytes\n";
			// ensure 'received' directory exists
			mkdir("received", 0755);

			string outpath = string("received/") + filename;
			int64_t offset = 0;
			ofstream ofs(outpath, ios::binary | ios::out);
			if (!ofs) {
				cerr << "ERROR: could not open " << outpath << " for writing\n";
			} else {
				const int CHUNK = MAX_MESSAGE; // server/client agreed buffer size
				//vector<char> recvbuf;
				//recvbuf.resize(CHUNK);
				vector<char> chunkbuf(MAX_MESSAGE);

				auto t0 = chrono::high_resolution_clock::now();

				while (offset < filesize) {
					int this_len = (int)min<int64_t>(CHUNK, filesize - offset);
					filemsg req(offset, this_len);
					int req_len = sizeof(filemsg) + filename.size() + 1;
					//char* rbuf = new char[req_len];
					vector<char> rbuf(req_len, 0);
					filemsg fr((int)offset, this_len);
					//memset(rbuf, 0, req_len);
					//memcpy(rbuf, &req, sizeof(filemsg));
					//strcpy(rbuf + sizeof(filemsg), filename.c_str());
					//chan.cwrite(rbuf, req_len);
					memcpy(rbuf.data(), &fr, sizeof(filemsg));
                    memcpy(rbuf.data() + sizeof(filemsg), filename.c_str(), filename.size() + 1);
                    chan.cwrite(rbuf.data(), req_len);
					//delete[] rbuf;

					// read the chunk (binary-safe)
					int received = 0;
					while (received < this_len) {
						int toread = this_len - received;
						// read into recvbuf.data() + received
						//chan.cread(recvbuf.data() + received, toread);
						chan.cread(chunkbuf.data() + received, toread);
						received += toread;
					}
					// write to file (binary)
					//ofs.write(recvbuf.data(), this_len);
					ofs.write(chunkbuf.data(), this_len);
					offset += this_len;
				}

				auto t1 = chrono::high_resolution_clock::now();
				chrono::duration<double> elapsed = t1 - t0;
				cout << "File transfer finished in " << elapsed.count() << " seconds\n";
				ofs.close();

				// After transfer, user can run: diff received/<filename> BIMDC/<filename>
				// to check identicalness.
			}
		}
	}

	MESSAGE_TYPE q = QUIT_MSG;
	for (auto ch : channels) {
		if (ch) {
			ch->cwrite(&q, sizeof(MESSAGE_TYPE));
			// Note: FIFORequestChannel destructor should close and unlink its FIFOs if appropriate
			delete ch;
		}
	}
	// if we created an extra data_chan (via NEWCHANNEL_MSG) and we added it to channels above,
	// it's already deleted above. If not in channels, delete now:
	if (data_chan && find(channels.begin(), channels.end(), data_chan) == channels.end()) {
		data_chan->cwrite(&q, sizeof(MESSAGE_TYPE));
		delete data_chan;
	}
	//char* buf3 = /*create buffer of size buff capacity (m)*/

	// loop over the segments in the file filesize / buff capacity
	// create filemsg instance
	//filemsg* file_req = (filemsg*)buf2;
	//file_req->offset = 0/*set offset in the file*/;
	//file_req->length = 0/*set the length. be careful of the last segment*/;
	// send the request (buf2)

	//chan.cwrite(buf2, len);

	// receive the response
	// cread into buf3 length file_req->length
	// write buf3 into file: received/filename

	//delete[] buf2;
	//delete[] buf3;

	// if necessary, close and delete the new channel
	
	/*if(new_chan){
		// do your close and delete
	}*/
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	return 0;

	int s = 0;
    if (waitpid(serverChild, &s, 0) == -1) {
        perror("waitpid");
    }
}
