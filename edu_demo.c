#include "types.h"
#include "stat.h"
#include "user.h"

static void
test_id_and_liveness(void)
{
  int id = edu_getid();
  printf(1, "EDU: ID reg = 0x%x\n", id);

  int l1 = edu_live_flip();
  int l2 = edu_live_flip();
  printf(1, "EDU: liveness flip 1 -> 0x%x, flip 2 -> 0x%x\n", l1, l2);
}

static void
test_raw_irq(void)
{
  int before = edu_intrcnt();
  printf(1, "EDU: IRQ count before raw IRQ = %d\n", before);

  printf(1, "EDU: raising raw IRQ with value 0x55...\n");
  edu_rawirq(0x55);

  sleep(10); // let ISR run

  int after = edu_intrcnt();
  int ist = edu_irqstatus();
  printf(1, "EDU: IRQ count after  raw IRQ = %d (delta=%d), last irq_status=0x%x\n",
         after, after - before, ist);
}

static void
test_factorial(void)
{
  int before = edu_intrcnt();

  for (int n = 0; n <= 10; n++) {
    int r = edu_fact(n);
    if (r < 0) {
      printf(1, "EDU: factorial(%d) failed\n", n);
      continue;
    }
    printf(1, "EDU: factorial(%d) = %d\n", n, r);
  }

  sleep(10); // allow IRQs to settle

  int after = edu_intrcnt();
  int ist = edu_irqstatus();
  printf(1, "EDU: IRQ count before fact = %d, after = %d (delta=%d)\n",
         before, after, after - before);
  printf(1, "EDU: last IRQ status after factorial tests = 0x%x\n", ist);
}

static void
test_dma(void)
{
  int before = edu_intrcnt();
  printf(1, "EDU: starting DMA roundtrip test...\n");

  int r = edu_dma_test();
  int after = edu_intrcnt();
  int ist = edu_irqstatus();

  if (r == 0)
    printf(1, "EDU: DMA roundtrip OK\n");
  else
    printf(1, "EDU: DMA roundtrip FAILED\n");

  printf(1, "EDU: IRQ count before DMA = %d, after = %d (delta=%d)\n",
         before, after, after - before);
  printf(1, "EDU: last IRQ status after DMA = 0x%x\n", ist);
}

int
main(int argc, char *argv[])
{
  printf(1, "=== EDU demo ===\n");

  char option[32];
  do {
      printf(1, "Choose an option:\n"
              "0: test_id_and_liveness\n"
              "1: test_raw_irq\n"
              "2: test_factorial\n"
              "3: test_dma\n");

      gets(option, 32);

      switch(option[0]) {
          case '0': test_id_and_liveness(); break;
          case '1': test_raw_irq(); break;
          case '2': test_factorial(); break;
          case '3': test_dma(); break;
          default:
                    printf(1, "unkown option %c\n", option[0]);
                    option[0] = '\0';
      }
  }  while (option[0]);

  printf(1, "=== EDU demo done ===\n");
  exit();
}

