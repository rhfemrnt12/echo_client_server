#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <thread>
#include <set>

using namespace std;

set<int> clients;

void usage() {
	cout << "syntax: ts [-an][-e[-b]] <port>\n";
	cout << "  -an: auto newline\n";
	cout << "  -e : echo\n";
	cout << "  -b : broadcast\n";
	cout << "sample: ts -b 1234\n";
}

struct Param {
	uint16_t port{0};
	bool autoNewline{false};
	bool echo{false};
	bool broadcast{false};
	bool isoption{false};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-an") == 0) {
				autoNewline = true;
				isoption = true;
				continue;
			}
			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				isoption = true;
				continue;
			}
			if (strcmp(argv[i], "-b") == 0) {
				broadcast = true;
				isoption = true;
				continue;
			}
		}
		port = stoi(argv[argc-1]);
		return port != 0;
	}
} param;

void recvThread(int sd) {
	cout << "connected\n";
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	while (true) {
		ssize_t res = recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			cerr << "recv return " << res << endl;
			clients.erase(sd);
			perror("recv");
			break;
		}
		buf[res] = '\0';
		if (param.autoNewline)
			cout << buf << endl;
		else {
			cout << buf;
			cout.flush();
		}
		if (param.echo) {
			cout << "\necho mode...\n";
			res = send(sd, buf, res, 0);
			if (res == 0 || res == -1) {
				cerr << "send return " << res << endl;
				clients.erase(sd);
				perror("send");
				break;
			}
		}
		else if(param.broadcast){
			cout << "\nbroadcast mode...\n";
			for(auto i = clients.begin(); i != clients.end(); i++){
				res = send(*i, buf, res, 0);
				if (res == 0 || res == -1) {
					cerr << "send return " << res << endl;
					clients.erase(sd);
					perror("send");
					break;
				}
			}
		}
	}
	cout << "disconnected\n";
    close(sd);
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}
	else if(param.isoption == false && argc > 2){
		usage();
		return -1;
	}

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	int optval = 1;
	int res = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(param.port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}

	res = listen(sd, 5);
	if (res == -1) {
		perror("listen");
		return -1;
	}

	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int cli_sd = accept(sd, (struct sockaddr *)&cli_addr, &len);
		if (cli_sd == -1) {
			perror("accept");
			break;
		}

		clients.insert(cli_sd);
		thread* t = new thread(recvThread, cli_sd);
		t->detach();
	}
	close(sd);
}
