#include <stdint.h>
#include "cmsis_os.h"
#include "printf.h"
#include "IRremote.h"

static ir_decode_results ir_results;


void ir_example_task(void const * argument)
{
  (void)argument;

  IR_enableIRIn();

  while(1) {
    if (IR_decode(&ir_results)) {
      printf("Code: 0x%08X | Type: %s | Address: 0x%02X\r\n",
        ir_results.value, IR_getProtocolString(&ir_results), ir_results.address);
#if 0
      printf("Raw: %d\r\n", results.rawlen - 1);
      printf("-----------------\r\n");
      for (uint16_t i = 1; i < results.rawlen; i++) {
        if (i > 1 && (i - 1) % 8 == 0) printf("\r\n");
        printf(i & 1 ? "+%04u " : "-%04u ", results.rawbuf[i] * MICROS_PER_TICK);
      }
      printf("\r\n-----------------\r\n");
#endif
      IR_resume();
    } else {
      osDelay(100);
    }
  }
}