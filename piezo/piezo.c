#include "piezo.h"

#define PIEZO_SENSOR 27
#define PIEZO_MAX_WAITING_TIME 85000

volatile bool piezo_falling_edge = false;

void pill_detection(uint gpio, uint32_t events){
    if(gpio == PIEZO_SENSOR)
        piezo_falling_edge = true;
}

void init_piezo(){
    gpio_init(PIEZO_SENSOR);
    gpio_set_dir(PIEZO_SENSOR, GPIO_IN);
    gpio_pull_up(PIEZO_SENSOR);
    gpio_set_irq_enabled_with_callback(PIEZO_SENSOR, GPIO_IRQ_EDGE_FALL, true, &pill_detection);
}

//returns a boolean if object is detected by piezo sensor within a maximum of waiting time
bool piezo_detection_within_us(){
    piezo_falling_edge = false;
    uint32_t time_start = time_us_64();
    do{
        if(piezo_falling_edge){
            return true;
        }
    }while((time_us_64() - time_start) <= PIEZO_MAX_WAITING_TIME);
    return false;
}
