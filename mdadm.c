#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"
#include "cache.h"

// Tracker to check the status of disks being mounted
int isMounted = 0;

// Builds the operation code according to the table in the prompt
uint32_t encode_op(int cmd, int disk_num, int block_num)
{
  uint32_t op = 0;
  op |= cmd << 26;
  op |= disk_num << 22;
  op |= block_num;
  return op;
}

// Obtains exact location of a byte based off of a given address
void translate_address(uint32_t addr, int *disk_num, int *block_num, int *offset)
{

  *disk_num = (addr / JBOD_DISK_SIZE);
  int temp;
  temp = addr % JBOD_DISK_SIZE;
  *block_num = (temp / JBOD_BLOCK_SIZE);
  *offset = temp % JBOD_BLOCK_SIZE;
}

// Mounts disks
int mdadm_mount(void)
{
  uint32_t op;
  if (isMounted == 0)
  {
    op = encode_op(JBOD_MOUNT, 0, 0);
    isMounted = 1;
    if (jbod_client_operation(op, NULL) == 0)
    {
      return 1;
    }
  }
  return -1;
}

// Unmounts disks
int mdadm_unmount(void)
{
  uint32_t op;
  if (isMounted == 1)
  {
    op = encode_op(JBOD_UNMOUNT, 0, 0);
    isMounted = 0;
    if (jbod_client_operation(op, NULL) == 0)
    {
      return 1;
    }
  }
  return -1;
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf)
{
  int disk_num;
  int block_num;
  int offset;
  int numsRead = 0;
  int bytesToRead = 0;

  while (len > 0)
  {
    translate_address(addr, &disk_num, &block_num, &offset);
    int cacheVal = -1;
    uint8_t buffer[256];

    // Fail instantly when given invalid parameters
    if (addr >= (JBOD_DISK_SIZE * JBOD_NUM_DISKS) || addr < 0 || len < 0 || len > 1024 || buf == 0 || isMounted == 0)
    {
      return -1;
    }
    else if (cache_enabled())
    {
      cacheVal = cache_lookup(disk_num, block_num, buffer);
    }
    if (cacheVal == -1)
    {

      // Find appropriate disk
      uint32_t op;
      int x;
      op = encode_op(JBOD_SEEK_TO_DISK, disk_num, 0);
      x = jbod_client_operation(op, NULL);
      if (x == -1)
      {
        return -1;
      }

      // Find appropriate block
      op = encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num);
      x = jbod_client_operation(op, NULL);
      if (x == -1)
      {
        return -1;
      }

      // Array to hold entire block at the location of the byte's address
      op = encode_op(JBOD_READ_BLOCK, 0, 0);
      x = jbod_client_operation(op, buffer);
      if (x == -1)
      {
        return -1;
      }
      if (cache_enabled())
        cache_insert(disk_num, block_num, buffer);
    }

    // Finds the amount of bytes left to read from the current block
    bytesToRead = 256 - offset;
    if (bytesToRead > len)
    {
      bytesToRead = len;
    }

    // Copies the bytes into input buffer and update address and other byte copying vars accordingly
    memcpy(buf + numsRead, buffer + offset, bytesToRead);
    numsRead += bytesToRead;
    len -= bytesToRead;
    addr += bytesToRead;
  }
  return numsRead;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf)
{
  int disk_num;
  int block_num;
  int offset;
  int numsWritten = 0;
  int bytesToWrite = 0;

  while (len > 0)
  {
    translate_address(addr, &disk_num, &block_num, &offset);
    uint8_t buffer[256];
    int cacheVal = -1;
    uint32_t op;
    int x;

    bytesToWrite = 256 - offset;
    if (bytesToWrite > len)
    {
      bytesToWrite = len;
    }

    // Fail instantly when given invalid parameters
    if (addr >= (JBOD_DISK_SIZE * JBOD_NUM_DISKS) || addr < 0 || len < 0 || len > 1024 || buf == 0)
    {
      return -1;
    }
    else if (cache_enabled())
    {
      cacheVal = cache_lookup(disk_num, block_num, buffer);
    }

    if (cacheVal == -1)
    {
      // Find appropriate disk
      op = encode_op(JBOD_SEEK_TO_DISK, disk_num, 0);
      x = jbod_client_operation(op, NULL);
      if (x == -1)
      {
        return -1;
      }

      // Find appropriate block
      op = encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num);
      x = jbod_client_operation(op, NULL);
      if (x == -1)
      {
        return -1;
      }

      op = encode_op(JBOD_READ_BLOCK, 0, 0);
      x = jbod_client_operation(op, buffer);
      if (x == -1)
      {
        return -1;
      }

      if (cache_enabled())
        cache_insert(disk_num, block_num, buffer);
    }

    op = encode_op(JBOD_SEEK_TO_DISK, disk_num, 0);
    x = jbod_client_operation(op, NULL);
    if (x == -1)
    {
      return -1;
    }

    // Find appropriate block
    op = encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num);
    x = jbod_client_operation(op, NULL);
    if (x == -1)
    {
      return -1;
    }

    memcpy(&buffer[offset], buf + numsWritten, bytesToWrite);

    op = encode_op(JBOD_WRITE_BLOCK, 0, 0);
    x = jbod_client_operation(op, buffer);
    if (x == -1)
    {
      return -1;
    }

    if (cache_enabled())
      cache_update(disk_num, block_num, buffer);

    // Copies the bytes into input buffer and update address and other byte copying vars accordingly
    numsWritten += bytesToWrite;
    len -= bytesToWrite;
    addr += bytesToWrite;
  }
  return numsWritten;
}
