clear; 
clg; 

load "tcp-xfer/tcp-xfer.out"; 
load "mpi-xfer/mpi-xfer.out";
load "rpc-xfer/rpc-xfer-0.out";
load "rpc-xfer/rpc-xfer-1.out";
load "portals-xfer/portals-xfer.out";
load "lwfs-xfer/lwfs-xfer-args-blk.out";
load "lwfs-xfer/lwfs-xfer-args-noblk.out";
load "lwfs-xfer/lwfs-xfer-data-blk.out";
load "lwfs-xfer/lwfs-xfer-data-noblk.out";

tcp_data = [tcp_xfer(:,3), tcp_xfer(:,8), tcp_xfer(:,11)];
mpi_data = [mpi_xfer(:,3), mpi_xfer(:,8), mpi_xfer(:,11)];
rpc_data_0 = [rpc_xfer_0(:,3), rpc_xfer_0(:,8), rpc_xfer_0(:,11)];
rpc_data_1 = [rpc_xfer_1(:,3), rpc_xfer_1(:,8), rpc_xfer_1(:,11)];
portals_data = [portals_xfer(:,3), portals_xfer(:,8), portals_xfer(:,11)];
lwfs_args_blk_data = [lwfs_xfer_args_blk(:,3), lwfs_xfer_args_blk(:,8), lwfs_xfer_args_blk(:,11)];
lwfs_args_noblk_data = [lwfs_xfer_args_noblk(:,3), lwfs_xfer_args_noblk(:,8), lwfs_xfer_args_noblk(:,11)];
lwfs_data_blk_data = [lwfs_xfer_data_blk(:,3), lwfs_xfer_data_blk(:,8), lwfs_xfer_data_blk(:,11)];
lwfs_data_noblk_data = [lwfs_xfer_data_noblk(:,3), lwfs_xfer_data_noblk(:,8), lwfs_xfer_data_noblk(:,11)];

%gplot data with linesp, data with errorbars;
%gplot \
%	tcp_data t "tcp" with linesp,  \
%	mpi_data t "mpi" with linesp, \
%	rpc_data t "rpc" with linesp, \
%	portals_data t "portals" with linesp, \
%	lwfs_args_blk_data t "lwfs-args-blk" with linesp, \
%	lwfs_data_blk_data t "lwfs-data-blk" with linesp
%gset logscale x; 
%gset xlabel "Data tranferred (bytes)";
%gset ylabel "Throughput (MB/s)";
%grid;
%gset key top left box 3

semilogx( \
	 tcp_data(:,1), tcp_data(:,2), '-@;tcp;', \
	 mpi_data(:,1), mpi_data(:,2), '-@;mpi;', \
	 rpc_data_0(:,1), rpc_data_0(:,2), '-@;rpc-0;', \
	 rpc_data_1(:,1), rpc_data_1(:,2), '-@;rpc-1;', \
	 portals_data(:,1), portals_data(:,2), '-@;portals;', \
	 lwfs_args_blk_data(:,1), lwfs_args_blk_data(:,2), '-@;lwfs-args;', \
	 lwfs_args_noblk_data(:,1), lwfs_args_noblk_data(:,2), '-@;lwfs-args-noblk;', \
	 lwfs_data_blk_data(:,1), lwfs_data_blk_data(:,2), '-@;lwfs-data;', \
	 lwfs_data_noblk_data(:,1), lwfs_data_noblk_data(:,2), '-@;lwfs-data-noblk;');

xlabel('Data tranferred (bytes)');
ylabel('Throughput (MB/s)');

gset key top left box 3;

gset term postscript eps color; 
gset output "plot.eps"; 
replot;

pause; 

%axis([10 10000 0 20]);
%pause; 
