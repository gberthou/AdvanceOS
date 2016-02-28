// Please see http://www.flipcode.com/archives/msys.c

#include "../mmu.h"
#include "../linker.h"
#include "../mem.h"

#define HEAP_BEGIN (GBA_HEAP_BEGIN + GBA_HEAP_SIZE)
#define HEAP_SIZE (64 << 20)

#define USED 1

typedef struct {
  unsigned size;
} UNIT;

typedef struct {
  UNIT* free;
  UNIT* heap;
} MSYS;

static MSYS msys;

static UNIT* compact( UNIT *p, unsigned nsize )
{
       unsigned bsize, psize;
       UNIT *best;

       best = p;
       bsize = 0;

       while( psize = p->size, psize )
       {
              if( psize & USED )
              {
                  if( bsize != 0 )
                  {
                      best->size = bsize;
                      if( bsize >= nsize )
                      {
                          return best;
                      }
                  }
                  bsize = 0;
                  best = p = (UNIT *)( (unsigned)p + (psize & ~USED) );
              }
              else
              {
                  bsize += psize;
                  p = (UNIT *)( (unsigned)p + psize );
              }
       }

       if( bsize != 0 )
       {
           best->size = bsize;
           if( bsize >= nsize )
           {
               return best;
           }
       }

       return 0;
}

void free( void *ptr )
{
     if( ptr )
     {
         UNIT *p;

         p = (UNIT *)( (unsigned)ptr - sizeof(UNIT) );
         p->size &= ~USED;
     }
}

void *malloc( unsigned size )
{
     unsigned fsize;
     UNIT *p;

     if( size == 0 ) return 0;

     size  += 3 + sizeof(UNIT);
     size >>= 2;
     size <<= 2;

     if( msys.free == 0 || size > msys.free->size )
     {
         msys.free = compact( msys.heap, size );
         if( msys.free == 0 ) return 0;
     }

     p = msys.free;
     fsize = msys.free->size;

     if( fsize >= size + sizeof(UNIT) )
     {
         msys.free = (UNIT *)( (unsigned)p + size );
         msys.free->size = fsize - size;
     }
     else
     {
         msys.free = 0;
         size = fsize;
     }

     p->size = size | USED;

     return (void *)( (unsigned)p + sizeof(UNIT) );
}

void USPiEnvAllocInit(void)
{
    msys.free = msys.heap = (UNIT *) HEAP_BEGIN;
    msys.free->size = msys.heap->size = HEAP_SIZE - sizeof(UNIT);
    *(unsigned *)((char *)HEAP_BEGIN + HEAP_SIZE - 4) = 0;

    // Set adequate MMU entries so that the whole USPI heap is mapped
    MMUPopulateRange(HEAP_BEGIN, HEAP_BEGIN, HEAP_SIZE, READWRITE);
}

void MSYS_Compact( void )
{
     msys.free = compact( msys.heap, 0x7FFFFFFF );
}

