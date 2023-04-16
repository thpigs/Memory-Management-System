#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

int cache_create(int num_entries)
{
  if (cache != NULL || num_entries < 2 || num_entries > 4096)
    return -1;
  else
  {
    cache = calloc(num_entries, sizeof(cache_entry_t));
    cache_size = num_entries;
  }

  return 1;
}

int cache_destroy(void)
{
  if (cache == NULL)
    return -1;
  else
    free(cache);
  cache = NULL;
  cache_size = 0;

  return 1;
}

bool isEmpty()
{
  for (int i = 0; i < cache_size; i++)
    if (cache[i].valid == true)
      return false;

  return true;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf)
{
  num_queries += 1;
  // Invalid parameters
  if (cache == NULL || buf == NULL || disk_num < 0 || block_num < 0 || disk_num > JBOD_NUM_DISKS || block_num > JBOD_NUM_BLOCKS_PER_DISK)
  {
    return -1;
  }
  else
    for (int i = 0; i < cache_size; i++)
    {
      if (!cache[i].valid)
      {
        return -1;
      }
      // Data found in cache
      if (cache[i].disk_num == disk_num && cache[i].block_num == block_num && cache[i].valid)
      {
        memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE);
        clock += 1;
        cache[i].access_time = clock;
        num_hits += 1;
        break;
      }
      // Reached end of cache without finding the data
      else if (isEmpty() || i == cache_size - 1)
      {
        return -1;
      }
    }
  return 1;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf)
{
  for (int i = 0; i < cache_size; i++)
  {
    if (!cache[i].valid)
    {
      break;
    }
    if (cache[i].disk_num == disk_num && cache[i].block_num == block_num && cache[i].valid)
    {
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
      cache[i].access_time = clock;
      break;
    }
  }
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf)
{

  // Invalid parameters
  if (cache == NULL || buf == NULL || disk_num < 0 || block_num < 0 || disk_num >= JBOD_NUM_DISKS || block_num >= JBOD_NUM_BLOCKS_PER_DISK)
  {
    return -1;
  }
  else
    for (int i = 0; i < cache_size; i++)
    {
      // Empty cache space available
      if (!cache[i].valid)
      {
        memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
        clock += 1;
        cache[i].access_time = clock;
        cache[i].valid = true;
        cache[i].block_num = block_num;
        cache[i].disk_num = disk_num;

        break;
      }
      // Repeat input
      else if (cache[i].disk_num == disk_num && cache[i].block_num == block_num && cache[i].access_time != 0)
      {
        return -1;
      }
      // Full cache
      else if (!isEmpty())
      {
        int replaceLRU = cache[0].access_time;
        int index = 0;
        for (int j = 1; j < cache_size; j++)
        {
          if (replaceLRU > cache[j].access_time)
          {
            replaceLRU = cache[j].access_time;
            index = j;
          }
        }
        memcpy(cache[index].block, buf, JBOD_BLOCK_SIZE);
        clock += 1;
        cache[index].access_time = clock;
        cache[index].valid = true;
        cache[index].block_num = block_num;
        cache[index].disk_num = disk_num;
        break;
      }
    }
  return 1;
}

bool cache_enabled(void)
{
  if (cache != NULL || cache_size != 0)
    return true;
  else
    return false;
}

void cache_print_hit_rate(void)
{
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float)num_hits / num_queries);
}
