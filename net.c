#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf)
{
  int numBytesRead = 0;
  while (numBytesRead < len)
  {
    int n = read(fd, &buf[numBytesRead], len - numBytesRead);
    if (n == -1)
    {
      return false;
    }
    else if (n == 0)
    {
      // end of file
    }
    else
    {
    }
  }
  return false;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf)
{
  return false;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block)
{
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block)
{
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port)
{
  /*
  Create socket
  Convert ip to binary form
  Connect
  */
  struct sockaddr_in caddr;
  int cli_sd = socket(AF_INET, SOCK_STREAM, 0);

  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(port);
  if (inet_aton(ip, &caddr.sin_addr) == 0)
  {
    int connection = connect(cli_sd, &caddr, sizeof(caddr));
    if (connection == 0)
    {
      return true;
    }
  }
  return false;
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void)
{
  /*
  close the connection
  */
  close(cli_sd);
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block)
{
  /*
  write packet;
  read response;
  */
}
