====================================================
---------------- Current  README -------------------
====================================================

Authors: Teng Ma, Biaodong He. @Tsinghua University, Madsys Group (http://madsys.cs.tsinghua.edu.cn/) 

# Code Informations:
Our project is based on DBX1000 which is produced by CMU.
The source code can be found here (https://github.com/mitdbg/deneva).

# The improvement
* The experiments (figures and tables) and baseline can be found in our paper (Section 4)

# How to install and test:
* Preprepared:
  - OFED-4.2 (libibverbs)
  - libmemcached-1.4.14
  - nanomsg-1.1.4 (only for our baseline)
  - jemalloc
* Compile: make deps && make -j
* Run: 
* Configurations: 
  - in `config.h`:
      - USE_RDMA: as the name shows
      - RDMA_BUFFER_SIZE: the circular buffer size
      - RDMA_CYC_QP_NUM: the depth of Send/Recv CQ
      - RDMA_BUFFER_ITEM_SIZE: buffer item size
      - RDMA_COLOR_LOG: for debugging
  - in `libmrdma.h`:
      - MAX_INLINE_DATA
      - MAX_SEND_SGE
      - MAX_RECV_SGE
      - M_CONN_PADDING: static offset value for UD mode
      - M_USE_SRQ: 
      - M_USE_MEMCACHE: use memcache to sync private informations of QP 
  - More details are described in the original README

# SW & HW info
```
Hardware:
CPU: Intel(R) Xeon(R) CPU E5-2640 v2 @ 2.00GHz
Network: Mellanox ConnectX-3 InfiniBand (40Gbps)
```
```
Software:
OFED: MLNX\_OFED\_LINUX-4.2-1.0.0.0 (OFED-4.2-1.0.0)
```
```
ibv_devices
hca_id: mlx4_0
        transport:                      InfiniBand (0)
        fw_ver:                         2.42.5000
        node_guid:                      f452:1403:0091:ba20
        sys_image_guid:                 f452:1403:0091:ba23
        vendor_id:                      0x02c9
        vendor_part_id:                 4099
        hw_ver:                         0x1
        board_id:                       MT_1090110018
        phys_port_cnt:                  2
        Device ports:
                port:   1
                        state:                  PORT_ACTIVE (4)
                        max_mtu:                4096 (5)
                        active_mtu:             4096 (5)
                        sm_lid:                 12
                        port_lid:               3
                        port_lmc:               0x00
                        link_layer:             InfiniBand

                port:   2
                        state:                  PORT_DOWN (1)
                        max_mtu:                4096 (5)
                        active_mtu:             1024 (3)
                        sm_lid:                 0
                        port_lid:               0
                        port_lmc:               0x00
                        link_layer:             Ethernet

```
```
ibstatus
Infiniband device 'mlx4_0' port 1 status:
        default gid:     fe80:0000:0000:0000:f452:1403:0091:ba11
        base lid:        0xa
        sm lid:          0xc
        state:           4: ACTIVE
        phys state:      5: LinkUp
        rate:            40 Gb/sec (4X QDR)
        link_layer:      InfiniBand

Infiniband device 'mlx4_0' port 2 status:
        default gid:     fe80:0000:0000:0000:f652:14ff:fe91:ba12
        base lid:        0x0
        sm lid:          0x0
        state:           1: DOWN
        phys state:      3: Disabled
        rate:            10 Gb/sec (1X QDR)
        link_layer:      Ethernet

```
```
link info:
CA: teaker-4 HCA-1:
      0xf45214030091b9f1      1    1[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       2    9[  ] "Infiniscale-IV Mellanox Technologies" ( )
CA: teaker-5 HCA-1:
      0xf45214030091baa1     12    1[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       2    7[  ] "Infiniscale-IV Mellanox Technologies" ( )
CA: teaker-6 HCA-1:
      0xf45214030091b9e1      8    1[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       2    6[  ] "Infiniscale-IV Mellanox Technologies" ( )
CA: teaker-7 HCA-1:
      0xf45214030091ba41      5    1[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       2    5[  ] "Infiniscale-IV Mellanox Technologies" ( )
CA: teaker-8 HCA-1:
      0xf45214030091ba61      7    1[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       2    4[  ] "Infiniscale-IV Mellanox Technologies" ( )
CA: teaker-11 HCA-1:
      0xf45214030091ba21      3    1[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       2    1[  ] "Infiniscale-IV Mellanox Technologies" ( )
CA: teaker-9 HCA-1:
      0xf45214030091ba51      6    1[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       2    3[  ] "Infiniscale-IV Mellanox Technologies" ( )
      0xf45214030091ba52      4    2[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       2   14[  ] "Infiniscale-IV Mellanox Technologies" ( )
Switch: 0x0002c902004b2678 Infiniscale-IV Mellanox Technologies:
           2    1[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       3    1[  ] "teaker-11 HCA-1" ( )
           2    2[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>      10    1[  ] "teaker-10 HCA-1" ( )
           2    3[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       6    1[  ] "teaker-9 HCA-1" ( )
           2    4[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       7    1[  ] "teaker-8 HCA-1" ( )
           2    5[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       5    1[  ] "teaker-7 HCA-1" ( )
           2    6[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       8    1[  ] "teaker-6 HCA-1" ( )
           2    7[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>      12    1[  ] "teaker-5 HCA-1" ( )
           2    8[  ] ==(                Down/ Polling)==>             [  ] "" ( )
           2    9[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       1    1[  ] "teaker-4 HCA-1" ( )
           2   10[  ] ==(                Down/ Polling)==>             [  ] "" ( )
           2   11[  ] ==(                Down/ Polling)==>             [  ] "" ( )
           2   12[  ] ==(                Down/ Polling)==>             [  ] "" ( )
           2   13[  ] ==(                Down/ Polling)==>             [  ] "" ( )
           2   14[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       4    2[  ] "teaker-9 HCA-1" ( )
           2   15[  ] ==(                Down/ Polling)==>             [  ] "" ( )
           2   16[  ] ==(                Down/ Polling)==>             [  ] "" ( )
           2   17[  ] ==(                Down/ Polling)==>             [  ] "" ( )
           2   18[  ] ==(                Down/ Polling)==>             [  ] "" ( )
CA: teaker-10 HCA-1:
      0xf45214030091ba11     10    1[  ] ==( 4X          10.0 Gbps Active/  LinkUp)==>       2    2[  ] "Infiniscale-IV Mellanox Technologies" ( )

```



--EOF--

====================================================
---------------- Original README -------------------
====================================================

DDBMS
=======

DDBMS is a testbed of an OLTP distributed database management system (DBMS). It supports 6 concurrency control algorithms.

This testbed is based on the DBx1000 system, whose concurrency control scalability study can be found in the following paper:

    Staring into the Abyss: An Evaluation of Concurrency Control with One Thousand Cores
    Xiangyao Yu, George Bezerra, Andrew Pavlo, Srinivas Devadas, Michael Stonebraker
    http://voltdb.com/downloads/datasheets_collateral/vdb_whitepaper_staring_into_the_abyss.pdf
    
Build & Test
------------

To build the database.

    make deps
    make -j

Configuration
-------------

DBMS configurations can be changed in the config.h file. Please refer to README for the meaning of each configuration. Here we only list several most important ones. 

    NODE_CNT          : Number of server nodes in the database
    THREAD_CNT        : Number of worker threads running per server
    WORKLOAD          : Supported workloads include YCSB and TPCC
    CC_ALG            : Concurrency control algorithm. Six algorithms are supported 
                        (NO_WAIT, WAIT_DIE, TIMESTAMP, MVCC, OCC, CALVIN) 
    MAX_TXN_IN_FLIGHT  : Maximum number of active transactions at each server at a given time
    DONE_TIMER        : Amount of time to run experiment
                        
Configurations can also be specified as command argument at runtime. Run the following command for a full list of program argument. 
    
    ./rundb -h

Run
---

The DBMS can be run with 

    ./rundb -nid[N]
    ./runcl -nid[M]

where N and M are the ID of a server and client, respectively
