/*
   Copyright 2016 Massachusetts Institute of Technology

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "global.h"
#include "manager.h"
#include "transport.h"
#ifdef USE_RDMA
  #include "nn.hpp"
  #include "sig_sync.hpp"
#else
  #include "nn_back.hpp"
#endif
#include "tpcc_query.h"
#include "query.h"
#include "message.h"


#define MAX_IFADDR_LEN 20 // max # of characters in name of address

void Transport::read_ifconfig(const char * ifaddr_file) {

	ifaddr = new char *[g_total_node_cnt];

	uint64_t cnt = 0;
  printf("Reading ifconfig file: %s\n",ifaddr_file);
	ifstream fin(ifaddr_file);
	string line;
  while (getline(fin, line)) {
		//memcpy(ifaddr[cnt],&line[0],12);
		ifaddr[cnt] = new char[line.length()+1];
    strcpy(ifaddr[cnt],&line[0]);
		printf("%ld: %s\n",cnt,ifaddr[cnt]);
		cnt++;
	}
  assert(cnt == g_total_node_cnt);
}

uint64_t Transport::get_socket_count() {
  uint64_t sock_cnt = 0;
  if(ISCLIENT)
    sock_cnt = (g_total_node_cnt)*2 + g_client_send_thread_cnt * g_servers_per_client;
  else
    sock_cnt = (g_total_node_cnt)*2 + g_client_send_thread_cnt;
  return sock_cnt;
}

string Transport::get_path() {
	string path;
#if SHMEM_ENV
  path = "/dev/shm/";
#else
	char * cpath;
  cpath = getenv("SCHEMA_PATH");
	if(cpath == NULL)
		path = "./";
	else
		path = string(cpath);
#endif
	path += "ifconfig.txt";
  return path;

}

Socket * Transport::get_socket() {
  //Socket * socket = new Socket;
  Socket * socket = (Socket*) mem_allocator.align_alloc(sizeof(Socket));
  new(socket) Socket();
	int timeo = 1000; // timeout in ms
	int stimeo = 1000; // timeout in ms
  int opt = 0;
  socket->sock.setsockopt(NN_SOL_SOCKET,NN_RCVTIMEO,&timeo,sizeof(timeo));
  socket->sock.setsockopt(NN_SOL_SOCKET,NN_SNDTIMEO,&stimeo,sizeof(stimeo));
  // NN_TCP_NODELAY doesn't cause TCP_NODELAY to be set -- nanomsg issue #118
  socket->sock.setsockopt(NN_SOL_SOCKET,NN_TCP_NODELAY,&opt,sizeof(opt));
  return socket;
}

uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id) {
  uint64_t port_id = TPORT_PORT;
  port_id += g_total_node_cnt * dest_node_id;
  port_id += src_node_id;
  DEBUG("Port ID:  %ld -> %ld : %ld\n",src_node_id,dest_node_id,port_id);
  return port_id;
}

#if NETWORK_DELAY_TEST || !ENVIRONMENT_EC2
uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id, uint64_t send_thread_id) {
  uint64_t port_id = 0;
  DEBUG("Calc port id %ld %ld %ld\n",src_node_id,dest_node_id,send_thread_id);
  port_id += g_total_node_cnt * dest_node_id;
  DEBUG("%ld\n",port_id);
  port_id += src_node_id;
  DEBUG("%ld\n",port_id);
//  uint64_t max_send_thread_cnt = g_send_thread_cnt > g_client_send_thread_cnt ? g_send_thread_cnt : g_client_send_thread_cnt;
//  port_id *= max_send_thread_cnt;
  port_id += send_thread_id * g_total_node_cnt * g_total_node_cnt;
  DEBUG("%ld\n",port_id);
  port_id += TPORT_PORT;
  DEBUG("%ld\n",port_id);
  printf("Port ID:  %ld, %ld -> %ld : %ld\n",send_thread_id,src_node_id,dest_node_id,port_id);
  return port_id;
}
#else

uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id, uint64_t send_thread_id) {
  uint64_t port_id = 0;
  DEBUG("Calc port id %ld %ld %ld\n",src_node_id,dest_node_id,send_thread_id);
  port_id += dest_node_id + src_node_id;
  DEBUG("%ld\n",port_id);
  port_id += send_thread_id * g_total_node_cnt * 2;
  DEBUG("%ld\n",port_id);
  port_id += TPORT_PORT;
  DEBUG("%ld\n",port_id);
  printf("Port ID:  %ld, %ld -> %ld : %ld\n",send_thread_id,src_node_id,dest_node_id,port_id);
  return port_id;
}
#endif



Socket * Transport::bind(uint64_t port_id) {
  Socket * socket = get_socket();
  char socket_name[MAX_TPORT_NAME];
#if TPORT_TYPE == IPC
  sprintf(socket_name,"ipc://node_%ld.ipc",port_id);
#else
#if ENVIRONMENT_EC2
  sprintf(socket_name,"tcp://eth0:%ld",port_id);
#else
  sprintf(socket_name,"tcp://%s:%ld",ifaddr[g_node_id],port_id);
#endif
#endif
  printf("Sock Binding to %s %d\n",socket_name,g_node_id);
#ifdef USE_RDMA
  int rc = socket->sock.bind(ifaddr[g_node_id], port_id);
#else
  int rc = socket->sock.bind(socket_name);
#endif
  printf("Sock Binding to %s %d End\n",socket_name,g_node_id);
  if(rc < 0) {
    printf("Bind Error: %d %s\n",errno,strerror(errno));
    assert(false);
  }
  return socket;
}

Socket * Transport::connect(uint64_t dest_id,uint64_t port_id) {
  Socket * socket = get_socket();
  char socket_name[MAX_TPORT_NAME];
#if TPORT_TYPE == IPC
  sprintf(socket_name,"ipc://node_%ld.ipc",port_id);
#else
#if ENVIRONMENT_EC2
  sprintf(socket_name,"tcp://eth0;%s:%ld",ifaddr[dest_id],port_id);
#else
  sprintf(socket_name,"tcp://%s;%s:%ld",ifaddr[g_node_id],ifaddr[dest_id],port_id);
#endif
#endif
  printf("Sock Connecting to %s %d -> %ld\n",socket_name,g_node_id,dest_id);
#ifdef USE_RDMA
  int rc = socket->sock.connect(ifaddr[g_node_id], port_id);
#else
  int rc = socket->sock.connect(socket_name);
#endif
  printf("Sock Connecting to %s %d -> %ld End\n",socket_name,g_node_id,dest_id);
  if(rc < 0) {
    printf("Connect Error: %d %s\n",errno,strerror(errno));
    assert(false);
  }
  return socket;
}

#ifdef USE_RDMA
void Transport::init() {
  _sock_cnt = get_socket_count();

  system("memcached -l 0.0.0.0 -p 10086 &");

  // m_nano_sleep(500000000);

#ifdef USE_IPC_SYNC
  sig_sync::IPCLock ipc = sig_sync::IPCLock(g_total_node_cnt);

  if (g_node_id == 0) {
    ipc.clear();
  }
#endif

  rr = 0;
	printf("Tport Init %d: %ld\n",g_node_id,_sock_cnt);

  vector<std::thread> th_bind;
  vector<std::thread> th_connect;

  string path = get_path();
	read_ifconfig(path.c_str());

  for(uint64_t node_id = 0; node_id < g_total_node_cnt; node_id++) {
    if(node_id == g_node_id)
      continue;
    // Listening ports
    if(ISCLIENTN(node_id)) {
      for(uint64_t client_thread_id = g_client_thread_cnt + g_client_rem_thread_cnt; client_thread_id < g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt; client_thread_id++) {

        th_bind.push_back(std::thread([=](){
          uint64_t port_id = get_port_id(node_id,g_node_id,client_thread_id % g_client_send_thread_cnt);
          Socket * sock = bind(port_id);
          recv_sockets.push_back(sock);
          DEBUG("[C%ld]Socket insert: {%ld}: %ld\n",client_thread_id,node_id,(uint64_t)sock);
          GREENLOG("[C%ld]Socket insert: {%ld}: %ld\n",client_thread_id,node_id,(uint64_t)port_id);

        }));
      }
    } else {
      for(uint64_t server_thread_id = g_thread_cnt + g_rem_thread_cnt; server_thread_id < g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt; server_thread_id++) {

        th_bind.push_back(std::thread([=](){
          uint64_t port_id = get_port_id(node_id,g_node_id,server_thread_id % g_send_thread_cnt);
          Socket * sock = bind(port_id);
          recv_sockets.push_back(sock);
          DEBUG("[S%ld]Socket insert: {%ld}: %ld\n",server_thread_id,node_id,(uint64_t)sock);
          GREENLOG("[S%ld]Socket insert: {%ld}: %ld\n",server_thread_id,node_id,(uint64_t)port_id);
          
        }));
      }
    }
  }

#ifdef USE_IPC_SYNC
  ipc.incre();
  ipc.wait();
  if (g_node_id == 0) {
    ipc.clear();
  }
#endif

  for(uint64_t node_id = 0; node_id < g_total_node_cnt; node_id++) {
    if(node_id == g_node_id)
      continue;
    // Sending ports
    if(ISCLIENTN(g_node_id)) {
      for(uint64_t client_thread_id = g_client_thread_cnt + g_client_rem_thread_cnt; client_thread_id < g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt; client_thread_id++) {

        th_connect.push_back(std::thread([=](){
          uint64_t port_id = get_port_id(g_node_id,node_id,client_thread_id % g_client_send_thread_cnt);
          std::pair<uint64_t,uint64_t> sender = std::make_pair(node_id,client_thread_id);
          Socket * sock = connect(node_id,port_id);
          send_sockets.insert(std::make_pair(sender,sock));
          DEBUG("[C%ld]Socket insert: {%ld}: %ld\n",client_thread_id,node_id,(uint64_t)sock);
          REDLOG("[C%ld]Socket insert: {%ld}: %ld\n",client_thread_id,node_id,(uint64_t)port_id);
        }));
      }
    } else {
      for(uint64_t server_thread_id = g_thread_cnt + g_rem_thread_cnt; server_thread_id < g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt; server_thread_id++) {

        th_connect.push_back(std::thread([=](){
          uint64_t port_id = get_port_id(g_node_id,node_id,server_thread_id % g_send_thread_cnt);
          std::pair<uint64_t,uint64_t> sender = std::make_pair(node_id,server_thread_id);
          Socket * sock = connect(node_id,port_id); 
          send_sockets.insert(std::make_pair(sender,sock));
          DEBUG("[S%ld]Socket insert: {%ld}: %ld\n",server_thread_id,node_id,(uint64_t)sock);
          REDLOG("[S%ld]Socket insert: {%ld}: %ld\n",server_thread_id,node_id,(uint64_t)port_id);
        }));
      }
    }
  }

  for (std::thread &th: th_connect) {
    th.join();
  }

  for (std::thread &th: th_bind) {
    th.join();
  }

#ifdef USE_IPC_SYNC
  ipc.incre();
  ipc.wait();
#endif

  REDLOG("/******** EVERYTHING IS OK ********/\n");

  REDLOG("SEND: %d\n", send_sockets.size());
  REDLOG("RECV: %d\n", recv_sockets.size());

  REDLOG("/******** EVERYTHING IS OK ********/\n");

  // for (auto &socket: send_sockets) {
  //   REDLOG("[%d -> %d]\n", socket.first.first, socket.first.second);
  //   socket.second->sock.print_pair_info_local();
  //   socket.second->sock.print_pair_info_remote();
  // }
  // for (auto &socket: recv_sockets) {
  //   // REDLOG("[%d -> %d]\n", socket.first.first, socket.first.second);
  //   socket->sock.print_pair_info_local();
  //   socket->sock.print_pair_info_remote();
  // }
  
  // for (auto &socket: send_sockets) {
  //   void * buf = socket.second->sock.get_send_buffer_addr();
  //   socket.second->sock.write(buf, 32, 0);
  // }

  // for (auto &socket: send_sockets) {
  //   printf("%s\n", (char *)(socket.second->sock.get_recv_buffer_addr()));
  // }


  // sleep(2);

  // exit(-1);

	fflush(stdout);
}
#else
void Transport::init() {
  _sock_cnt = get_socket_count();

  rr = 0;
  printf("Tport Init %d: %ld\n",g_node_id,_sock_cnt);

  string path = get_path();
  read_ifconfig(path.c_str());

  for(uint64_t node_id = 0; node_id < g_total_node_cnt; node_id++) {
    if(node_id == g_node_id)
      continue;
    // Listening ports
    if(ISCLIENTN(node_id)) {
      for(uint64_t client_thread_id = g_client_thread_cnt + g_client_rem_thread_cnt; client_thread_id < g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt; client_thread_id++) {
        uint64_t port_id = get_port_id(node_id,g_node_id,client_thread_id % g_client_send_thread_cnt);
        Socket * sock = bind(port_id);
        recv_sockets.push_back(sock);
        DEBUG("[C%ld]Socket insert: {%ld}: %ld\n",client_thread_id,node_id,(uint64_t)sock);
        GREENLOG("[C%ld]Socket insert: {%ld}: %ld\n",client_thread_id,node_id,(uint64_t)port_id);
      }
    } else {
      for(uint64_t server_thread_id = g_thread_cnt + g_rem_thread_cnt; server_thread_id < g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt; server_thread_id++) {
        uint64_t port_id = get_port_id(node_id,g_node_id,server_thread_id % g_send_thread_cnt);
        Socket * sock = bind(port_id);
        recv_sockets.push_back(sock);
        DEBUG("[S%ld]Socket insert: {%ld}: %ld\n",server_thread_id,node_id,(uint64_t)sock);
        GREENLOG("[S%ld]Socket insert: {%ld}: %ld\n",server_thread_id,node_id,(uint64_t)port_id);

      }
    }
    // Sending ports
    if(ISCLIENTN(g_node_id)) {
      for(uint64_t client_thread_id = g_client_thread_cnt + g_client_rem_thread_cnt; client_thread_id < g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt; client_thread_id++) {
        uint64_t port_id = get_port_id(g_node_id,node_id,client_thread_id % g_client_send_thread_cnt);
        std::pair<uint64_t,uint64_t> sender = std::make_pair(node_id,client_thread_id);
        Socket * sock = connect(node_id,port_id);
        send_sockets.insert(std::make_pair(sender,sock));
        DEBUG("[C%ld]Socket insert: {%ld}: %ld\n",client_thread_id,node_id,(uint64_t)sock);
        REDLOG("[C%ld]Socket insert: {%ld}: %ld\n",client_thread_id,node_id,(uint64_t)port_id);

      }
    } else {
      for(uint64_t server_thread_id = g_thread_cnt + g_rem_thread_cnt; server_thread_id < g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt; server_thread_id++) {
        uint64_t port_id = get_port_id(g_node_id,node_id,server_thread_id % g_send_thread_cnt);
        std::pair<uint64_t,uint64_t> sender = std::make_pair(node_id,server_thread_id);
        Socket * sock = connect(node_id,port_id);
        send_sockets.insert(std::make_pair(sender,sock));
        DEBUG("[S%ld]Socket insert: {%ld}: %ld\n",server_thread_id,node_id,(uint64_t)sock);
        REDLOG("[S%ld]Socket insert: {%ld}: %ld\n",server_thread_id,node_id,(uint64_t)port_id);
      }
    }
  }


  fflush(stdout);
}
#endif

// rename sid to send thread id
void Transport::send_msg(uint64_t send_thread_id, uint64_t dest_node_id, void * sbuf,int size) {
  uint64_t starttime = get_sys_clock();

  // REDLOG("[Send_Size]%d\n", size);

  Socket * socket = send_sockets.find(std::make_pair(dest_node_id,send_thread_id))->second;
  // Copy messages to nanomsg buffer
#ifdef USE_RDMA
  void * buf = socket->sock.get_send_buffer_addr();
#else
	void * buf = nn::allocmsg(size,0);
#endif
	memcpy(buf,sbuf,size);
  DEBUG("%ld Sending batch of %d bytes to node %ld on socket %ld\n",send_thread_id,size,dest_node_id,(uint64_t)socket);

  int rc = -1;
  while(rc < 0 && (!simulation->is_setup_done() || (simulation->is_setup_done() && !simulation->is_done()))) {
#ifdef USE_RDMA
    rc= socket->sock.send(buf,size,NN_DONTWAIT);
#else
    rc= socket->sock.send(&buf,NN_MSG,NN_DONTWAIT);
#endif

  }
  //nn_freemsg(sbuf);
  DEBUG("%ld Batch of %d bytes sent to node %ld\n",send_thread_id,size,dest_node_id);

  INC_STATS(send_thread_id,msg_send_time,get_sys_clock() - starttime);
  INC_STATS(send_thread_id,msg_send_cnt,1);
}

// Listens to sockets for messages from other nodes
std::vector<Message*> * Transport::recv_msg(uint64_t thd_id) {
	int bytes = 0;
	void * buf;
  uint64_t starttime = get_sys_clock();
  std::vector<Message*> * msgs = NULL;
  //uint64_t ctr = starttime % recv_sockets.size();
  uint64_t rand = (starttime % recv_sockets.size()) / g_this_rem_thread_cnt;
  //uint64_t ctr = ((thd_id % g_this_rem_thread_cnt) % recv_sockets.size()) + rand * g_this_rem_thread_cnt;
  uint64_t ctr = thd_id % g_this_rem_thread_cnt;
  if(ctr >= recv_sockets.size())
    return msgs;
  if(g_this_rem_thread_cnt < g_total_node_cnt) {
    ctr += rand * g_this_rem_thread_cnt;
    while(ctr >= recv_sockets.size()) {
      ctr -= g_this_rem_thread_cnt;
    }
  }
  assert(ctr < recv_sockets.size());
  uint64_t start_ctr = ctr;
  
	
  while(bytes <= 0 && (!simulation->is_setup_done() || (simulation->is_setup_done() && !simulation->is_done()))) {
    Socket * socket = recv_sockets[ctr];
		bytes = socket->sock.recv(&buf, NN_MSG, NN_DONTWAIT);

#ifdef USE_RDMA
    // GREENLOG("[Recv_Size]%d\n", bytes);
#endif
    //++ctr;
    ctr = (ctr + g_this_rem_thread_cnt);

    if(ctr >= recv_sockets.size())
      ctr = (thd_id % g_this_rem_thread_cnt) % recv_sockets.size();
    if(ctr == start_ctr)
      break;

#ifdef USE_RDMA
      // Do nothing
#else
		if(bytes <= 0 && errno != 11) {
		  printf("Recv Error %d %s\n",errno,strerror(errno));
    }
		nn::freemsg(buf);	
#endif

		if(bytes>0)
			break;
	}

	if(bytes <= 0 ) {
    INC_STATS(thd_id,msg_recv_idle_time, get_sys_clock() - starttime);
    return msgs;
	}

  INC_STATS(thd_id,msg_recv_time, get_sys_clock() - starttime);
	INC_STATS(thd_id,msg_recv_cnt,1);

	starttime = get_sys_clock();

  msgs = Message::create_messages((char*)buf);
  DEBUG("Batch of %d bytes recv from node %ld; Time: %f\n",bytes,msgs->front()->return_node_id,simulation->seconds_from_start(get_sys_clock()));

#ifdef USE_RDMA
      // Do nothing
#else
  nn::freemsg(buf); 
#endif
  
	INC_STATS(thd_id,msg_unpack_time,get_sys_clock()-starttime);
  return msgs;
}

/*
void Transport::simple_send_msg(int size) {
	void * sbuf = nn_allocmsg(size,0);

	ts_t time = get_sys_clock();
	memcpy(&((char*)sbuf)[0],&time,sizeof(ts_t));

  int rc = -1;
  while(rc < 0 ) {
  if(g_node_id == 0)
    rc = socket->sock.send(&sbuf,NN_MSG,0);
  else
    rc = socket->sock.send(&sbuf,NN_MSG,0);
	}
}

uint64_t Transport::simple_recv_msg() {
	int bytes;
	void * buf;

  if(g_node_id == 0)
		bytes = socket->sock.recv(&buf, NN_MSG, NN_DONTWAIT);
  else
		bytes = socket->sock.recv(&buf, NN_MSG, NN_DONTWAIT);
    if(bytes <= 0 ) {
      if(errno != 11)
        nn::freemsg(buf);	
      return 0;
    }

	ts_t time;
	memcpy(&time,&((char*)buf)[0],sizeof(ts_t));
	//printf("%d bytes, %f s\n",bytes,((float)(get_sys_clock()-time)) / BILLION);

	nn::freemsg(buf);	
	return bytes;
}
*/

