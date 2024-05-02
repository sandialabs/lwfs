#!/bin/sh -x

attr_mgr --set --sysarch suse-9.0-x86_64 --vmname lustre-server --image lustre-base/vmlinuz n0.o0

sudo mk_conf --local --dhcp n0.o0
sudo mk_conf --pxe n0.o0
sudo build_diskless --force n0.o0
