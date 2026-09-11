#ifndef RF_SDL_JOYSTICK_ADAPTER_H
#define RF_SDL_JOYSTICK_ADAPTER_H

#include <stdint.h>

/* Minimal DirectInput-shaped state consumed by the historical runtime. */
typedef struct DIJOYSTATE2 {
    int32_t lX;
    int32_t lY;
    int32_t lZ;
    uint8_t rgbButtons[128];
} DIJOYSTATE2;

class Joystick {
public:
    explicit Joystick(unsigned int device_index);
    ~Joystick();

    static unsigned int deviceCount();
    static void setNeutralInput(bool enabled);
    bool open();
    void close();
    bool poll(DIJOYSTATE2 *state);

private:
    unsigned int device_index_;
    struct _SDL_Joystick *device_;

    Joystick(const Joystick &);
    Joystick &operator=(const Joystick &);
};

#endif
