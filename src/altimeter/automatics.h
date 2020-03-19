// Altimeter modes (altimeter_mode)
#define MODE_ON_EARTH 0
#define MODE_IN_AIRPLANE 1
#define MODE_EXIT 2
#define MODE_BEGIN_FREEFALL 3
#define MODE_FREEFALL 4
#define MODE_PULLOUT 5
#define MODE_OPENING 6
#define MODE_UNDER_PARACHUTE 7
#define MODE_PREFILL 8
#define MODE_DUMB 9
#define MODE_IN_AIR (altimeter_mode > MODE_ON_EARTH && altimeter_mode < MODE_PREFILL)
#define MODE_IN_JUMP (altimeter_mode > MODE_IN_AIRPLANE && altimeter_mode < MODE_PREFILL)

#define CLEAR_PREVIOUS_ALTITUDE -30000

void processAltitude();
