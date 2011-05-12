#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <stdint.h>
#include <sys/socket.h>
#include <pthread.h>
#include "transport.h"
#include "../handler/message_buffer.h"
#include "macros.h"
#include "../localregistry/daemon/registry_error.h"
#include "../localregistry/user/registry_ops.h"

#define DEFAULT_TRANSPORT_OUTGOING_MESSAGE_BUFFER_SIZE 20

typedef struct __tp(app_handler){
  uint8_t id;

  __tp(buffer) * outgoing_messages;
  
  struct {
    int unicast_fd;
    int broadcast_fd;
    fd_set active_fds;
  }sockets;

  struct{
    pthread_t receiver; 
    pthread_t sender;
  }threads;

  struct{//Callbacks
	void (*create_transport_cb)(struct __tp(app_handler)* sk);
    int8_t (*send_cb)(struct __tp(app_handler)* sk, char* data, uint16_t len, uint8_t dst_id);
    void (*receive_cb)(struct __tp(app_handler)* sk, char* data, uint16_t len, int64_t timestamp, uint8_t src_id);
    int8_t (*close_handler_cb)(struct __tp(app_handler)* sk);
    void* control_data;
  }transport;

  struct{
    registry_descriptor* regd;
  }module_communication;

  struct{
	  uint8_t unicast:1,
		  broadcast:1,
		  reserved:6;
  }flags;
}__tp(handler);

extern __tp(handler)* __tp(create_handler_based_on_file)(char* filename, void (*receive_cb)(struct __tp(app_handler)*, char*, uint16_t, int64_t, uint8_t));
extern __tp(handler)* __tp(create_handler_based_on_string)(char* xml_string, void (*receive_cb)(struct __tp(app_handler)*, char*, uint16_t, int64_t, uint8_t));
extern void __tp(free_handler)(__tp(handler)* sk);

/*Applications should use these macros to invoke each function.*/
#define send_data(handler, data, data_len, dst_id)	\
  (handler)->transport.send_cb((handler), (data), (data_len), (dst_id))

#define close_transport_handler(handler)				\
  do{									\
    if((handler)->transport.close_handler_cb)					\
      (handler)->transport.close_handler_cb((handler));				\
  }while(0)

#endif
