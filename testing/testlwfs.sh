#!/bin/sh


# Checkout the latest version
mkdir -p ${HOME}/tmp/testing
cd ${HOME}/tmp/testing
svn co svn+ssh://raoldfi@software.sandia.gov/home/raoldfi/repos/lwfs/trunk lwfs; 
# Build the configure script
cd lwfs; 
autogen.sh; 

# Configure LWFS
cd build/i386;
sh init-configure.sh;

# Make LWFS
make; 

# Call the correctness tests
cd testing; 
make check; 

