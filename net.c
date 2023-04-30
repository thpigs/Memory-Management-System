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
    numBytesRead += n;
  }
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf)
{
  int numBytesWritten = 0;
  while (numBytesWritten < len)
  {
    int n = write(fd, &buf[numBytesWritten], len - numBytesWritten);
    if (n == -1)
    {
      return false;
    }
    numBytesWritten += n;
  }
  return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block)
{
  /*
  uint8_t header[8];
  if (nread() is successful) {

  }
  Reading from a socket, do a bunch of reads.
  Need to read in steps. First need to read in header (8bytes).
  Same thing as send_pack, but first read header length from block/socket.
  Then parse header depending on the length. It may need to read the complete block instead of just the header.
  */
  if (*ret == -1)
  {
    return false;
  }
  uint8_t header[8];
  memcpy(header, block, sizeof(uint8_t) * 8);

  int len = HEADER_LEN;
  if (*op == JBOD_WRITE_BLOCK || *op == JBOD_READ_BLOCK)
  {
    len += JBOD_BLOCK_SIZE;
  }
  if (nread(cli_sd, len, block) == true)
  {
    return true;
  }
  return false;
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block)
{
  /*
  nwrite()
  Just make a single call to nwrite(),
  First create send buffer with header length depending on what command is being sent. Ex. seek commands don't need a buffer, but a write command does
  Then call nwrite(). done.
  */
  /*
   uint16_t len = HEADER_LEN;

   if (op >> 26 == JBOD_WRITE_BLOCK)
   {
     len += JBOD_BLOCK_SIZE;
   }

   len = htons(len);
   uint8_t buf[HEADER_LEN + JBOD_BLOCK_SIZE];
   memcpy(buf, &len, sizeof(len));
   len = ntohs(len);

   op = htonl(op);
   // buf and len are different byte values, so this might the destination might not be the right starting point.
   memcpy(&buf[2], &op, sizeof(op));

   if (block != NULL)
   {
     memcpy(&buf[8], block, JBOD_BLOCK_SIZE * sizeof(uint8_t));
   }

   if (nwrite(cli_sd, len, buf) == false)
   {
     return false;
   }
   return true;
   */

  uint16_t len = HEADER_LEN;
  // Write and read commands go here. Buf will either be empty or have contents and len will be 264.
  if (op >> 26 == JBOD_WRITE_BLOCK || op >> 26 == JBOD_READ_BLOCK)
  {
    len += JBOD_BLOCK_SIZE;
    uint8_t buf[HEADER_LEN + JBOD_BLOCK_SIZE];

    len = htons(len);
    memcpy(buf, &len, sizeof(len));
    len = ntohs(len);

    op = htonl(op);
    memcpy(&buf[2], &op, sizeof(op));
    memcpy(&buf[8], block, JBOD_BLOCK_SIZE * sizeof(uint8_t));

    if (nwrite(cli_sd, len, buf) == false)
    {
      return false;
    }
  }
  // Mount and seek commands go here. Buf will be NULL and len will be 8.
  else
  {
    uint8_t buf[HEADER_LEN];
    len = htons(len);
    memcpy(buf, &len, sizeof(len));
    len = ntohs(len);

    op = htonl(op);
    memcpy(&buf[2], &op, sizeof(op));

    if (nwrite(cli_sd, len, buf) == false)
    {
      return false;
    }
  }

  return true;
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

  // Creates socket
  int cli_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (cli_sd == -1)
  {
    return false;
  }

  // Specify address
  struct sockaddr_in caddr;
  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(port);
  if (inet_aton(ip, &caddr.sin_addr) == 0)
  {
    return false;
  }

  // Establish connection
  int connection = connect(cli_sd, (const struct sockaddr *)&caddr, sizeof(caddr));
  if (connection == -1)
  {
    return false;
  }
  return true;
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void)
{
  /*
  close the connection
  */
  close(cli_sd);
  cli_sd = -1;
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block)
{
  /*
  send_packet()
  recv_packet()
  write packet;
  read response;

  maybe recv_packet is for only read, send_packet is for all others. So check op here and do the according call.
  */

  bool x = send_packet(cli_sd, op, block);
  if (x == false)
  {
    return -1;
  }
  uint16_t retCode[2];
  memcpy(retCode, &block[6], 2 * sizeof(uint16_t));
  x = recv_packet(cli_sd, &op, retCode, block);
  if (x == false)
  {
    return -1;
  }
  return 0;

  /*
    bool x;
    if (op >> 26 == JBOD_READ_BLOCK)
    {
      uint16_t retCode[2];
      memcpy(retCode, &block[6], 2 * sizeof(uint16_t));
      x = recv_packet(cli_sd, &op, retCode, block);
      if (!x)
      {
        return -1;
      }
    }
    else
    {
      x = send_packet(cli_sd, op, block);
      if (!x)
      {
        return -1;
      }
    }
    return 0;
    */
}
