#include "setup.h"

void app_main(void) {
    if (!setup_check()) {
        setup_run();  // Startet Setup-Modus mit Loop
    } else {
        // Normaler Betriebsmodus
        // ... dein Code ...
    }
}
