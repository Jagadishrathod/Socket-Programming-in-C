#include<stdio.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>

int c_socket;
int s_socket, s_server;

int s_background_socket, s_background_server;

struct Device
{
	char device_name[100];
	char ip_addr[100];
	char subnetid[100];
	char port[20];
	int connected_to_this;
};

struct RoutingTable
{
	struct Device devices[4];
};

int ServerCreate(int port)		// Return 1 for error
{
	s_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server, other;
	memset(&server, 0, sizeof(server));
	memset(&other, 0, sizeof(other));

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = INADDR_ANY;


	if(bind(s_socket, (struct sockaddr*)&server, sizeof(server)) != -1)
	{
		printf("Server Running.....\n");
		listen(s_socket, 5);

		return 0;

		//int accepterror = AcceptNewClient();
		//return accepterror;
	}
	else
	{
		//printf("\nError in Bind.\n");
		return 1;
	}
}

struct sockaddr_in other;

void * AcceptNewClient(void * p)
{
	//struct sockaddr_in other;
	memset(&other, 0, sizeof(other));
	socklen_t add = sizeof(other);

	s_server = accept(s_socket, (struct sockaddr*)&other, &add);
	if(s_server == -1) ;//return 1;
	else
	{
		printf("\n[+] Conection accepted from %s, %d\n",inet_ntoa(other.sin_addr),ntohs(other.sin_port));
		//return 0;
	}

	pthread_exit(0);
}

int ClientCreate(int port, char IPADDR[])		// Return 1 for error
{
	c_socket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in client;
	memset(&client, 0, sizeof(client));
	client.sin_family = AF_INET;
	client.sin_port = htons(port);
	client.sin_addr.s_addr = inet_addr(IPADDR);
	//client.sin_addr.s_addr = INADDR_ANY;
	if(connect(c_socket, (struct sockaddr *)&client, sizeof(client)) == -1)
	{
		printf("Connection Issue.\n");
		return 1;
	}
	else return 0;
}

struct RoutingTable rt;

void RoutingTableInit(char devicenames[][100], char ipaddrs[][100], char subnetid[][100], char ports[][20], int no_of_devices, char this_device_name[], int connections[][100])
{
	int this_device_index = 0;
	for(int i=0;i<no_of_devices;i++)
	{
		if(strcmp(devicenames[i], this_device_name) == 0) this_device_index = i;
	}

	for(int i=0;i<no_of_devices;i++)
	{
		strcpy(rt.devices[i].device_name, devicenames[i]);
		strcpy(rt.devices[i].ip_addr, ipaddrs[i]);
		strcpy(rt.devices[i].subnetid, subnetid[i]);
		strcpy(rt.devices[i].port, ports[i]);
		if(connections[this_device_index][i] == 1 && connections[i][this_device_index] == 1) rt.devices[i].connected_to_this = 1;
		else rt.devices[i].connected_to_this = 0;
	}
}

int GetRoutingTableIndex(char device_name[], int no_of_devices)
{
	for(int i=0;i<no_of_devices;i++)
	{
		if(strcmp(rt.devices[i].device_name, device_name) == 0) return i;
	}
	return -1;
}

int no_of_bits(int no_of_devices)
{
	int bits = 0;
	while(no_of_devices > 0)
	{
		bits++;
		no_of_devices /= 2;
		if(no_of_devices == 1) break;
	}
	return bits;
}

char GetClassID(char ip[])
{
	char class;

	int classid = (ip[2]-48) + 10*(ip[1]-48) + 100*(ip[0]-48);
	if(classid >= 0 && classid <= 127) class = 'a';
	if(classid >= 128 && classid <= 191) class = 'b';
	if(classid >= 192 && classid <= 223) class = 'c';
	if(classid >= 224 && classid <= 239) class = 'd';
	if(classid >= 240 && classid <= 254) class = 'e';

	return class;
}

int GetIPPart(int bits)
{
	int ippart = 0;
	int inc = 128;
	while(bits > 0)
	{
		ippart += inc;
		inc /= 2;
		bits--;
	}
	return ippart;
}

int main()
{
	int no_of_devices = 4;
	int subnet_bits = no_of_bits(no_of_devices);

	int host_bits = 0;

	char devicenames[/*no_of_devices*/][100] = {"A", "B", "C", "D"};
	char ipaddrs[/*no_of_devices*/][100] = {"127.0.0.1", "127.0.0.1", "127.0.0.1", "127.0.0.1"};
	char subnetid[/*no_of_devices*/][100] = {"127.0.0.0", "127.64.0.0", "127.128.0.1", "127.192.0.1"};
	char ports[/*no_of_devices*/][20] = {"9009", "9010", "9011", "9012"};
	int connections[/*no_of_devices*/][100] = {
													 {-1, 1, 0, 0},
													 {1, -1, 0, 1},
													 {0, 0, -1, 1},
													 {0, 1, 1, -1}
	};

	char this_device_name[100];
	printf("Enter this Device name: ");
	scanf("%s", this_device_name);

	RoutingTableInit(devicenames, ipaddrs, subnetid, ports, no_of_devices, this_device_name, connections);

	int self_index = GetRoutingTableIndex(this_device_name, no_of_devices);

	printf("\n------------------------NODE %s---------------------------\n", this_device_name);
	printf("\nName: %s, IP: %s, Port: %s\n", this_device_name, rt.devices[self_index].ip_addr, rt.devices[self_index].port);

	//Create Server
	//int port = 9009;
	//printf("Enter port: ");
	//scanf("%d", &port);
	int errors = ServerCreate(atoi(rt.devices[self_index].port));
	if(errors == 1)
	{
		close(s_server);
		close(s_socket);
		return 0;
	}

	pthread_t tid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&tid, &attr, AcceptNewClient, NULL);

	//init vars
	int choice = 0;
	printf("Do you want to send(0/1): ");
	scanf("%d", &choice);

	if(choice == 1)
	{
		char text[500];
		char dest_deviceIP[100];
		char dest_devicename[100];

		printf("Enter Destination Device IP: ");
		scanf("%s", dest_deviceIP);

		char subnetmask[100];
		char class = GetClassID(dest_deviceIP);

		printf("\nCLASS: %c\n", class);
		printf("\nSUBNETBITS: %d\n", subnet_bits);

		if(class == 'a')
		{
			strcpy(subnetmask, "255.");
			if(subnet_bits <= 8)
			{
				char itoatemp[100];
				sprintf(itoatemp, "%d", GetIPPart(subnet_bits));
				strcat(subnetmask, itoatemp);
				strcat(subnetmask, ".0.0");
			}
			else if(subnet_bits <= 16)
			{
				char itoatemp[100];
				strcat(subnetmask, "255.");
				sprintf(itoatemp, "%d", GetIPPart(subnet_bits - 8));
				strcat(subnetmask, itoatemp);
				strcat(subnetmask, ".0");
			}
			else if(subnet_bits <= 24)
			{
				char itoatemp[100];
				strcat(subnetmask, "255.255.");
				sprintf(itoatemp, "%d", GetIPPart(subnet_bits - 16));
				strcat(subnetmask, itoatemp);
			}
		}
		else if(class == 'b')
		{
			strcpy(subnetmask, "255.255.");
			if(subnet_bits <= 8)
			{
				char itoatemp[100];
				sprintf(itoatemp, "%d", GetIPPart(subnet_bits));
				strcat(subnetmask, itoatemp);
				strcat(subnetmask, ".0");
			}
			else if(subnet_bits <= 16)
			{
				char itoatemp[100];
				strcat(subnetmask, "255.");
				sprintf(itoatemp, "%d", GetIPPart(subnet_bits - 8));
				strcat(subnetmask, itoatemp);
			}
		}
		else if(class == 'c')
		{
			strcpy(subnetmask, "255.255.255.");
			if(subnet_bits <= 8)
			{
				char itoatemp[100];
				sprintf(itoatemp, "%d", GetIPPart(subnet_bits));
				strcat(subnetmask, itoatemp);
			}
		}

		printf("\nSUBNETMASK: %s\n", subnetmask);

		int subnetid_val[4];
		for(int i=0;i<4;i++)
		{
			int temp = 0;
			int a = (dest_deviceIP[i*4 + 2]-48) + 10*(dest_deviceIP[i*4 + 1]-48) + 100*(dest_deviceIP[i*4 + 0]-48);
			int b = (subnetmask[i*4 + 2]-48) + 10*(subnetmask[i*4 + 1]-48) + 100*(subnetmask[i*4 + 0]-48);
			while(a > 0 || b > 0)
			{
				temp = 10*temp + (a%2 && b%2);
				if(a == 0) break;
				if(b == 0) break;
				a /= 2;
				b /= 2;
			}
			int temp2 = 0;
			while(temp > 0)
			{
				temp2 = 10*temp2 + temp%10;
				temp /= 10;
			}
			temp = temp2;
			int val = 1;
			while(temp > 0)
			{
				subnetid_val[i] += val*(temp%10);
				temp /= 10;
				if(temp == 0) break;
				val *= 2;
			}
		}
		char dest_subnetid[100];
		char subnettemp[100];
		sprintf(subnettemp, "%d", subnetid_val[0]);
		strcpy(dest_subnetid, subnettemp);
		strcat(dest_subnetid, ".");
		sprintf(subnettemp, "%d", subnetid_val[1]);
		strcat(dest_subnetid, subnettemp);
		strcat(dest_subnetid, ".");
		sprintf(subnettemp, "%d", subnetid_val[2]);
		strcat(dest_subnetid, subnettemp);
		strcat(dest_subnetid, ".");
		sprintf(subnettemp, "%d", subnetid_val[3]);
		strcat(dest_subnetid, subnettemp);

		printf("\nSUBNETID DEST: %s\n", dest_subnetid);

/*
		printf("Enter IP Address to send: ");
		scanf("%s", rt.ip_addr);
*/
		printf("Enter text to send: ");
		scanf("%s", text);

		int this_device_index = GetRoutingTableIndex(this_device_name, no_of_devices);
		int dest_device_index = -1;

		for(int i=0;i<no_of_devices;i++)
		{
			if(strcmp(rt.devices[i].subnetid, dest_subnetid) == 0) dest_device_index = i;
		}

		if(dest_device_index != -1) strcpy(dest_devicename, rt.devices[dest_device_index].device_name);

		if(this_device_index != -1 && dest_device_index != -1 && this_device_index != dest_device_index)
		{
			if(rt.devices[dest_device_index].connected_to_this == 1)
			{
				errors = ClientCreate(atoi(rt.devices[dest_device_index].port), rt.devices[dest_device_index].ip_addr);
				if(errors == 1)
				{
					printf("\nERROR in Connection to %s.\n", rt.devices[dest_device_index].device_name);
					close(c_socket);
					//return 0;
				}
				else
				{
					send(c_socket, rt.devices[this_device_index].device_name, sizeof(rt.devices[this_device_index].device_name), 0);
					send(c_socket, rt.devices[this_device_index].ip_addr, sizeof(rt.devices[this_device_index].ip_addr), 0);
					send(c_socket, rt.devices[this_device_index].port, sizeof(rt.devices[this_device_index].port), 0);
					send(c_socket, rt.devices[dest_device_index].device_name, sizeof(rt.devices[dest_device_index].device_name), 0);
					send(c_socket, rt.devices[dest_device_index].ip_addr, sizeof(rt.devices[dest_device_index].ip_addr), 0);
					send(c_socket, rt.devices[dest_device_index].port, sizeof(rt.devices[dest_device_index].port), 0);
					send(c_socket, text, sizeof(text), 0);
					close(c_socket);
				}
			}
			else
			{
				for(int i=0;i<no_of_devices;i++)
				{
					if(rt.devices[i].connected_to_this == 1)
					{
						errors = ClientCreate(atoi(rt.devices[i].port), rt.devices[i].ip_addr);
						if(errors == 1)
						{
							printf("\nERROR in Connection to %s.\n", rt.devices[i].device_name);
							close(c_socket);
							//return 0;
						}
						else
						{
							send(c_socket, rt.devices[this_device_index].device_name, sizeof(rt.devices[this_device_index].device_name), 0);
							send(c_socket, rt.devices[this_device_index].ip_addr, sizeof(rt.devices[this_device_index].ip_addr), 0);
							send(c_socket, rt.devices[this_device_index].port, sizeof(rt.devices[this_device_index].port), 0);
							send(c_socket, rt.devices[dest_device_index].device_name, sizeof(rt.devices[dest_device_index].device_name), 0);
							send(c_socket, rt.devices[dest_device_index].ip_addr, sizeof(rt.devices[dest_device_index].ip_addr), 0);
							send(c_socket, rt.devices[dest_device_index].port, sizeof(rt.devices[dest_device_index].port), 0);
							send(c_socket, text, sizeof(text), 0);
							close(c_socket);
						}
					}
				}
			}
		}
	}
	else
	{
		printf("\nWaiting for data......\n");

		pthread_join(tid, NULL);

		char src_devicename[100];
		char dest_devicename[100];
		char dest_ipaddr[100];
		char src_ipaddr[100];
		char dest_port[20];
		char src_port[20];
		char dest_text[500];

		recv(s_server, src_devicename, sizeof(src_devicename), 0);
		recv(s_server, src_ipaddr, sizeof(src_ipaddr), 0);
		recv(s_server, src_port, sizeof(src_port), 0);
		recv(s_server, dest_devicename, sizeof(dest_devicename), 0);
		recv(s_server, dest_ipaddr, sizeof(dest_ipaddr), 0);
		recv(s_server, dest_port, sizeof(dest_port), 0);
		recv(s_server, dest_text, sizeof(dest_text), 0);

		strcpy(src_ipaddr, inet_ntoa(other.sin_addr));

		// printf("\nConnection SRC: src_devicename: %s, src_ipaddr: %s, src_port: %s\n", src_devicename, src_ipaddr, src_port);
		// printf("\nConnection DEST: dest_devicename: %s, dest_ipaddr: %s, dest_port: %s\n", dest_devicename, dest_ipaddr, dest_port);
		// printf("\nConnection TEXT: %s\n", dest_text);

		if(strcmp(dest_devicename, this_device_name) == 0)
		{
			printf("Device %s with IP %s sent text: %s", src_devicename, dest_ipaddr, dest_text);
		}
		else
		{
			int this_device_index = GetRoutingTableIndex(this_device_name, no_of_devices);
			int dest_device_index = GetRoutingTableIndex(dest_devicename, no_of_devices);

			if(this_device_index != -1 && dest_device_index != -1 && this_device_index != dest_device_index)
			{
				if(rt.devices[dest_device_index].connected_to_this == 1)
				{
					errors = ClientCreate(atoi(dest_port), rt.devices[dest_device_index].ip_addr);
					if(errors == 1)
					{
						printf("\nERROR in Connection to %s.\n", dest_devicename);
						close(c_socket);
						//return 0;
					}
					else
					{
						send(c_socket, rt.devices[this_device_index].device_name, sizeof(rt.devices[this_device_index].device_name), 0);
						send(c_socket, rt.devices[this_device_index].ip_addr, sizeof(rt.devices[this_device_index].ip_addr), 0);
						send(c_socket, rt.devices[this_device_index].port, sizeof(rt.devices[this_device_index].port), 0);
						send(c_socket, dest_devicename, sizeof(dest_devicename), 0);
						send(c_socket, dest_ipaddr, sizeof(dest_ipaddr), 0);
						send(c_socket, dest_port, sizeof(dest_port), 0);
						send(c_socket, dest_text, sizeof(dest_text), 0);
						close(c_socket);
					}
				}
				else
				{
					for(int i=0;i<no_of_devices;i++)
					{
						if(rt.devices[i].connected_to_this == 1)
						{
							if(!(strcmp(rt.devices[i].device_name, src_devicename)) && !(strcmp(rt.devices[i].ip_addr, src_ipaddr)))
							{
								errors = ClientCreate(atoi(rt.devices[i].port), rt.devices[i].ip_addr);
								if(errors == 1)
								{
									printf("\nERROR in Connection to %s.\n", rt.devices[i].device_name);
									close(c_socket);
									//return 0;
								}
								else
								{
									send(c_socket, rt.devices[this_device_index].device_name, sizeof(rt.devices[this_device_index].device_name), 0);
									send(c_socket, rt.devices[this_device_index].ip_addr, sizeof(rt.devices[this_device_index].ip_addr), 0);
									send(c_socket, rt.devices[this_device_index].port, sizeof(rt.devices[this_device_index].port), 0);
									send(c_socket, dest_devicename, sizeof(dest_devicename), 0);
									send(c_socket, dest_ipaddr, sizeof(dest_ipaddr), 0);
									send(c_socket, dest_port, sizeof(dest_port), 0);
									send(c_socket, dest_text, sizeof(dest_text), 0);
									close(c_socket);
								}
							}
						}
					}
				}
			}
		}
	}
	printf("\n------------------------------------------------------------\n");
	return 0;
}
