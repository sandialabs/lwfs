# Lightweight File System (LWFS)

The Lightweight File System (LWFS) is a storage system that provides a minimal set of I/O-system functionality required by file system and/or I/O library implementations for massively parallel machines. In particular, the LWFS-core consists of a scalable security model, an efficient data-movement protocol, and a direct interface to object-based storage devices. Higher-level services such as namespace management, consistency semantics, reliability, and so forth are layered on top of the core services to provide application-specific functionality as needed. The LWFS code contains implementations of the core services and reference implementations of a number of supplemental services for namespace management and transaction support.

SCR #1166
