410ULIB_IPC_OBJS := ipc_client.o ipc_server.o 
410ULIB_IPC_OBJS := $(410ULIB_IPC_OBJS:%=$(410UDIR)/libipc/%)

ALL_410UOBJS += $(410ULIB_IPC_OBJS)
410UCLEANS += $(410UDIR)/libipc.a

$(410UDIR)/libipc.a: $(410ULIB_IPC_OBJS)
