// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13
#define HASH(id) (id % NBUCKETS)
struct hashbuf {
  struct buf head;       // 头节点
  struct spinlock lock;  // 锁
};


struct {
  //struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct hashbuf hashbucket[NBUCKETS]; //每个哈希队列一个linked list及一个lock
} bcache;

void
binit(void)
{
  struct buf *b;

  char lockname[16];

  for(int i = 0; i < NBUCKETS; ++i) {
    // 初始化散列桶的自旋锁
    snprintf(lockname, sizeof(lockname), "bcache_%d", i);
    initlock(&bcache.hashbucket[i].lock, lockname);

    // 初始化散列桶的头节点
    bcache.hashbucket[i].head.prev = &bcache.hashbucket[i].head;
    bcache.hashbucket[i].head.next = &bcache.hashbucket[i].head;
  }
  /*initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;*/
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    // 利用头插法初始化缓冲区列表,全部放到散列桶0上
    b->next = bcache.hashbucket[0].head.next;
    b->prev = &bcache.hashbucket[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[0].head.next->prev = b;
    bcache.hashbucket[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int cur=HASH(blockno);//当前块的blocknumber
  acquire(&bcache.hashbucket[cur].lock);

  // Is the block already cached?
  for(b = bcache.hashbucket[cur].head.next; b != &bcache.hashbucket[cur].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;

      //使用时间戳
      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);

      release(&bcache.hashbucket[cur].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  int cnt=0,i=cur;
  struct buf* tmp;
  b=0;
  while(cnt<NBUCKETS)
  {
    i++;
    i%=NBUCKETS;
    cnt++;
    if(i==cur)
    {
      continue;
    }
    if(holding(&bcache.hashbucket[i].lock))
    {
      continue;
    }
    acquire(&bcache.hashbucket[i].lock);
    for(tmp = bcache.hashbucket[i].head.prev; tmp != &bcache.hashbucket[i].head; tmp= tmp->prev){
      if(tmp->refcnt == 0 && (b == 0 || tmp->timestamp < b->timestamp)) {
        b=tmp;
      }
    }
    if(b) 
    {
      // 如果是从其他散列桶窃取的，则将其以头插法插入到当前桶
      if(i != cur) {
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.hashbucket[i].lock);

        b->next = bcache.hashbucket[cur].head.next;
        b->prev = &bcache.hashbucket[cur].head;
        bcache.hashbucket[cur].head.next->prev = b;
        bcache.hashbucket[cur].head.next = b;
      }

      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);

      release(&bcache.hashbucket[cur].lock);
      acquiresleep(&b->lock);
      return b;
    } 
    else 
    {
      // 在当前散列桶中未找到，则直接释放锁
      if(i != cur)
        release(&bcache.hashbucket[i].lock);
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  int cur=HASH(b->blockno);
  releasesleep(&b->lock);

  acquire(&bcache.hashbucket[cur].lock);
  b->refcnt--;
  acquire(&tickslock);
  b->timestamp = ticks;
  release(&tickslock);
  
  release(&bcache.hashbucket[cur].lock);
}

void
bpin(struct buf *b) {
  int cur = HASH(b->blockno);
  acquire(&bcache.hashbucket[cur].lock);
  b->refcnt++;
  release(&bcache.hashbucket[cur].lock);
}

void
bunpin(struct buf *b) {
  int cur= HASH(b->blockno);
  acquire(&bcache.hashbucket[cur].lock);
  b->refcnt--;
  release(&bcache.hashbucket[cur].lock);
}


