#include <pigpio.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    int PWM_pin_a;
    int PWM_pin_b;
    int encoder_pin_a;
    int encoder_pin_b;

    volatile int position;
    volatile int previous_position;
    volatile int error_count;

    volatile int state_a;
    volatile int previous_state_a;
    volatile int state_b;
    volatile int previous_state_b;

    double last_tick;
    double debounce_time;

    volatile int running;
    pthread_t polling_thread;
} Motor;

void* polling(void* arg) {
    Motor* motor = (Motor*)arg;

    while (motor->running) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        double current_time = ts.tv_sec + ts.tv_nsec / 1e9;

        motor->state_a = gpioRead(motor->encoder_pin_a);
        motor->state_b = gpioRead(motor->encoder_pin_b);

        if ((motor->state_a != motor->previous_state_a) || (motor->state_b != motor->previous_state_b)) {
            motor->last_tick = current_time;

            if (((motor->state_a == 0 && motor->state_b == 1) && (motor->previous_state_a == 1 && motor->previous_state_b == 1)) ||
                ((motor->state_a == 0 && motor->state_b == 0) && (motor->previous_state_a == 0 && motor->previous_state_b == 1)) ||
                ((motor->state_a == 1 && motor->state_b == 0) && (motor->previous_state_a == 0 && motor->previous_state_b == 0)) ||
                ((motor->state_a == 1 && motor->state_b == 1) && (motor->previous_state_a == 1 && motor->previous_state_b == 0))) {
                motor->position++;
            }

            if (((motor->state_a == 0 && motor->state_b == 1) && (motor->previous_state_a == 0 && motor->previous_state_b == 0)) ||
                ((motor->state_a == 0 && motor->state_b == 0) && (motor->previous_state_a == 1 && motor->previous_state_b == 0)) ||
                ((motor->state_a == 1 && motor->state_b == 0) && (motor->previous_state_a == 1 && motor->previous_state_b == 1)) ||
                ((motor->state_a == 1 && motor->state_b == 1) && (motor->previous_state_a == 0 && motor->previous_state_b == 1))) {
                motor->position--;
            }

            motor->previous_state_a = motor->state_a;
            motor->previous_state_b = motor->state_b;

            if (motor->position - motor->previous_position != 1) {
                motor->error_count++;
                printf("Position: %d /////////////////////////////////\n", motor->position);
            } else {
                printf("Position: %d\n", motor->position);
            }

            motor->previous_position = motor->position;
        }

        //usleep(10); // Sleep for 100 microseconds
    }

    return NULL;
}

void start_polling(Motor* motor) {
    motor->running = 1;
    pthread_create(&motor->polling_thread, NULL, polling, motor);
}

void stop_polling(Motor* motor) {
    motor->running = 0;
    pthread_join(motor->polling_thread, NULL);
}

void forward(Motor* motor, int hz, int duty_cycle) {
    gpioHardwarePWM(motor->PWM_pin_a, hz, duty_cycle);
    gpioWrite(motor->PWM_pin_b, 0);
}

int forward_with_wind_down(Motor* motor, int hz, int initial_duty_cycle, int target_position, int chunks) {
    
    int wind_down_start_position = target_position - (target_position*0.2);

    while(motor->position < wind_down_start_position){
        gpioHardwarePWM(motor->PWM_pin_a, hz, initial_duty_cycle);
        gpioWrite(motor->PWM_pin_b, 0);
    }

    int remaining_steps = target_position - motor->position;
    int remaining_duty = initial_duty_cycle - 100000;

    for(int i = 1; i <= chunks; i++){

        while( motor->position < ( wind_down_start_position + ((remaining_steps/chunks)*i) - motor->error_count ) ){
            gpioHardwarePWM(motor->PWM_pin_a, hz, initial_duty_cycle - ((remaining_duty/chunks)*i));
            gpioWrite(motor->PWM_pin_b, 0);
        }
    }

    while( motor->position < target_position - motor->error_count ){
            gpioHardwarePWM(motor->PWM_pin_a, hz, initial_duty_cycle - ((remaining_duty/chunks)*chunks));
            gpioWrite(motor->PWM_pin_b, 0);

    }

    gpioWrite(motor->PWM_pin_a, 0);
    gpioWrite(motor->PWM_pin_b, 0);

}

void backward(Motor* motor, int hz, int duty_cycle) {
    gpioHardwarePWM(motor->PWM_pin_b, hz, duty_cycle);
    gpioWrite(motor->PWM_pin_a, 0);
}

int main() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialization failed\n");
        return 1;
    }

    Motor motor1 = {18, 12, 17, 27, 0, 0, 0, 1, 0, 0, 0, 0, 0.001, 0};

    gpioSetMode(motor1.PWM_pin_a, PI_OUTPUT);
    gpioSetMode(motor1.PWM_pin_b, PI_OUTPUT);
    gpioSetMode(motor1.encoder_pin_a, PI_INPUT);
    gpioSetMode(motor1.encoder_pin_b, PI_INPUT);

    start_polling(&motor1);

    int result = forward_with_wind_down(&motor1, 2000, 600000, 1485, 7);
    
    //result = forward_with_wind_down(&motor1, 2000, 400000, 1485*2, 7);     
    
    //result = forward_with_wind_down(&motor1, 2000, 200000, 1485*3, 7);     

    /*
    while (motor1.position <= 1150) {
        forward(&motor1, 2000, 700000);
    }
    */

    gpioWrite(18, 0);
    gpioWrite(12, 0);
    printf("Error count: %d\n", motor1.error_count);

    stop_polling(&motor1);
    gpioTerminate();

    return 0;
}
