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
    if (n < 0)
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
    if (n < 0)
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
  uint16_t len;
  uint8_t buf[HEADER_LEN];

  if (nread(cli_sd, HEADER_LEN, buf) == false)
  {
    return false;
  }

  memcpy(&len, &buf, sizeof(len));
  memcpy(&op, &buf[2], sizeof(op));
  memcpy(&ret, &buf[6], sizeof(ret));
  len = ntohs(len);
  op = ntohl(op);
  ret = ntohs(ret);

  if (len > HEADER_LEN)
  {
    if (nread(cli_sd, JBOD_BLOCK_SIZE, block) == false)
    {
      return false;
    }
  }
  return true;
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block)
{
  uint16_t len = HEADER_LEN;
  uint16_t retCode = 0;
  if (op >> 26 == JBOD_WRITE_BLOCK)
  {
    len += JBOD_BLOCK_SIZE;
    uint8_t buf[HEADER_LEN + JBOD_BLOCK_SIZE];

    len = htons(len);
    memcpy(buf, &len, sizeof(len));
    len = ntohs(len);

    op = htonl(op);
    memcpy(&buf[2], &op, sizeof(op));

    retCode = htons(retCode);
    memcpy(&buf[6], &retCode, sizeof(retCode));

    memcpy(&buf[8], block, JBOD_BLOCK_SIZE * sizeof(uint8_t));

    if (nwrite(cli_sd, len, buf) == false)
    {
      return false;
    }
  }
  else
  {
    uint8_t buf[HEADER_LEN];
    len = htons(len);
    memcpy(buf, &len, sizeof(len));
    len = ntohs(len);

    op = htonl(op);
    memcpy(&buf[2], &op, sizeof(op));

    retCode = htons(retCode);
    memcpy(&buf[6], &retCode, sizeof(retCode));

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
  // Specify address
  struct sockaddr_in caddr;
  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(port);
  if (inet_aton(ip, &caddr.sin_addr) == 0)
  {
    return false;
  }

  // Creates socket
  cli_sd = socket(AF_INET, SOCK_STREAM, 0);

  if (cli_sd == -1)
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
  close(cli_sd);
  cli_sd = -1;
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block)
{
  bool x = send_packet(cli_sd, op, block);
  if (x == false)
  {
    return -1;
  }

  uint16_t retCode;
  uint32_t retOp;
  x = recv_packet(cli_sd, &retOp, &retCode, block);
  if (x == false)
  {
    return -1;
  }

  if (retCode == -1)
  {
    return -1;
  }

  return 0;
}
