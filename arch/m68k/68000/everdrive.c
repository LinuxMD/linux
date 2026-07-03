#include <asm/everdrive.h>

unsigned long everdrive_random_get_entropy(void)
{
        void *reg = (void *) MEGADRIVE_EVERDRIVE_TIMER;

        return ioread16be(reg);
}
