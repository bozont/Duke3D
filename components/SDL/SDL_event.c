
#include <stdlib.h>
#include <driver/uart.h>
#include "SDL_event.h"
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

typedef struct {
	int gpio;
	SDL_Scancode scancode;
    SDL_Keycode keycode;
} GPIOKeyMap;

int keyMode = 1;
//Mappings from buttons to keys
#ifdef CONFIG_HW_ODROID_GO
static const GPIOKeyMap keymap[2][6]={{
// Game    
	{CONFIG_HW_BUTTON_PIN_NUM_BUTTON1, SDL_SCANCODE_LCTRL, SDLK_LCTRL},			
	{CONFIG_HW_BUTTON_PIN_NUM_SELECT, SDL_SCANCODE_SPACE, SDLK_SPACE},	
	{CONFIG_HW_BUTTON_PIN_NUM_VOL, SDL_SCANCODE_CAPSLOCK, SDLK_CAPSLOCK},		
	{CONFIG_HW_BUTTON_PIN_NUM_MENU, SDL_SCANCODE_ESCAPE, SDLK_ESCAPE},	
	{CONFIG_HW_BUTTON_PIN_NUM_START, SDL_SCANCODE_A, SDLK_a},	
	{CONFIG_HW_BUTTON_PIN_NUM_BUTTON2, SDL_SCANCODE_LALT, SDLK_LALT},	
},
// Menu
{
	{CONFIG_HW_BUTTON_PIN_NUM_BUTTON1, SDL_SCANCODE_SPACE, SDLK_SPACE}, 	
	{CONFIG_HW_BUTTON_PIN_NUM_BUTTON2, SDL_SCANCODE_ESCAPE, SDLK_ESCAPE},			
	{CONFIG_HW_BUTTON_PIN_NUM_VOL, SDL_SCANCODE_ESCAPE, SDLK_ESCAPE},	    
	{CONFIG_HW_BUTTON_PIN_NUM_MENU, SDL_SCANCODE_ESCAPE, SDLK_ESCAPE},	
	{CONFIG_HW_BUTTON_PIN_NUM_START, SDL_SCANCODE_A, SDLK_a},	
	{CONFIG_HW_BUTTON_PIN_NUM_SELECT, SDL_SCANCODE_LALT, SDLK_LALT},	
}};
#else
static const GPIOKeyMap keymap[2][6]={{
// Game    
	{CONFIG_HW_BUTTON_PIN_NUM_UP, SDL_SCANCODE_UP, SDLK_UP},
	{CONFIG_HW_BUTTON_PIN_NUM_RIGHT, SDL_SCANCODE_RIGHT, SDLK_RIGHT},
 	{CONFIG_HW_BUTTON_PIN_NUM_DOWN, SDL_SCANCODE_DOWN, SDLK_DOWN},
	{CONFIG_HW_BUTTON_PIN_NUM_LEFT, SDL_SCANCODE_LEFT, SDLK_LEFT}, 

	{CONFIG_HW_BUTTON_PIN_NUM_BUTTON1, SDL_SCANCODE_LCTRL, SDLK_LCTRL},			
	{CONFIG_HW_BUTTON_PIN_NUM_BUTTON2, SDL_SCANCODE_SPACE, SDLK_SPACE},		
},
// Menu
{
	{CONFIG_HW_BUTTON_PIN_NUM_UP, SDL_SCANCODE_UP, SDLK_UP},
	{CONFIG_HW_BUTTON_PIN_NUM_RIGHT, SDL_SCANCODE_RIGHT, SDLK_RIGHT},
    {CONFIG_HW_BUTTON_PIN_NUM_DOWN, SDL_SCANCODE_DOWN, SDLK_DOWN},
	{CONFIG_HW_BUTTON_PIN_NUM_LEFT, SDL_SCANCODE_LEFT, SDLK_LEFT}, 

	{CONFIG_HW_BUTTON_PIN_NUM_BUTTON2, SDL_SCANCODE_ESCAPE, SDLK_ESCAPE},			
	{CONFIG_HW_BUTTON_PIN_NUM_BUTTON1, SDL_SCANCODE_SPACE, SDLK_SPACE},   	
}};
#endif

#define ODROID_GAMEPAD_IO_X ADC1_CHANNEL_6
#define ODROID_GAMEPAD_IO_Y ADC1_CHANNEL_7

typedef struct
{
    uint8_t up;
    uint8_t right;
    uint8_t down;
    uint8_t left;
    uint8_t buttons[6];
} JoystickState;

JoystickState lastState = {0,0,0,0,{0,0,0,0,0,0}};

typedef struct {
    Uint32 type;        /**< ::SDL_KEYDOWN or ::SDL_KEYUP */
    SDL_Scancode scancode;
    SDL_Scancode keycode;
} GPIOEvent;

bool initInput = false;

static xQueueHandle gpio_evt_queue = NULL;
static xQueueHandle uart_evt_queue = NULL;

int checkPin(int state, uint8_t *lastState, SDL_Scancode sc, SDL_Keycode kc, SDL_Event *event)
{
    if(state != *lastState)
    {
        *lastState = state;
        event->key.keysym.scancode = sc;
        event->key.keysym.sym = kc;
        event->key.type = state ? SDL_KEYDOWN : SDL_KEYUP;
        event->key.state = state ? SDL_PRESSED : SDL_RELEASED;
        return 1;
    }
    return 0;
}

int checkPinStruct(int i, uint8_t *lastState, SDL_Event *event)
{
    int state = 1-gpio_get_level(keymap[keyMode][i].gpio);
    if(state != *lastState)
    {
        *lastState = state;
        event->key.keysym.scancode = keymap[keyMode][i].scancode;
        event->key.keysym.sym = keymap[keyMode][i].keycode;
        event->key.type = state ? SDL_KEYDOWN : SDL_KEYUP;
        event->key.state = state ? SDL_PRESSED : SDL_RELEASED;
        return 1;
    }
    return 0;
}

int readOdroidXY(SDL_Event * event)
{
    int joyX = adc1_get_raw(ODROID_GAMEPAD_IO_X);
    int joyY = adc1_get_raw(ODROID_GAMEPAD_IO_Y);
    
    JoystickState state;
    if (joyX > 2048 + 1024)
    {
        state.left = 1;
        state.right = 0;
    }
    else if (joyX > 1024)
    {
        state.left = 0;
        state.right = 1;
    }
    else
    {
        state.left = 0;
        state.right = 0;
    }

    if (joyY > 2048 + 1024)
    {
        state.up = 1;
        state.down = 0;
    }
    else if (joyY > 1024)
    {
        state.up = 0;
        state.down = 1;
    }
    else
    {
        state.up = 0;
        state.down = 0;
    }    
    
    event->key.keysym.mod = 0;
    if(checkPin(state.up, &lastState.up, SDL_SCANCODE_UP, SDLK_UP, event))
        return 1;
    if(checkPin(state.down, &lastState.down, SDL_SCANCODE_DOWN, SDLK_DOWN, event))
        return 1;
    if(checkPin(state.left, &lastState.left, SDL_SCANCODE_LEFT, SDLK_LEFT, event))
        return 1;
    if(checkPin(state.right, &lastState.right, SDL_SCANCODE_RIGHT, SDLK_RIGHT, event))
        return 1;

    for(int i = 0; i < 6; i++)
        if(checkPinStruct(i, &lastState.buttons[i], event))
            return 1;

    return 0;
}

int SDL_PollEvent(SDL_Event * event)
{
    if(!initInput)
        inputInit();

    GPIOEvent ev;

    if(xQueueReceive(uart_evt_queue, &ev, 0)) {
        event->key.keysym.sym = ev.keycode;
        event->key.keysym.scancode = ev.scancode;
        event->key.type = ev.type;
        event->key.keysym.mod = 0;
        event->key.state = ev.type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;     //< ::SDL_PRESSED or ::SDL_RELEASED
        return 1;
    }

#ifndef CONFIG_HW_ODROID_GO
    if(xQueueReceive(gpio_evt_queue, &ev, 0)) {
        event->key.keysym.sym = ev.keycode;
        event->key.keysym.scancode = ev.scancode;
        event->key.type = ev.type;
        event->key.keysym.mod = 0;
        event->key.state = ev.type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;     //< ::SDL_PRESSED or ::SDL_RELEASED 
        return 1;
    }
#else
    return readOdroidXY(event);
#endif
    return 0;
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    GPIOEvent event;
    event.type = gpio_get_level(gpio_num) == 0 ? SDL_KEYDOWN : SDL_KEYUP;
    for (int i=0; i < NELEMS(keymap[keyMode]); i++)
        if(keymap[keyMode][i].gpio == gpio_num)
        {
            event.scancode = keymap[keyMode][i].scancode;
            event.keycode = keymap[keyMode][i].keycode;
            xQueueSendFromISR(gpio_evt_queue, &event, NULL);
        }
}
/*
void gpioTask(void *arg) {
    uint32_t io_num;
	int level;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
        }
    }
}
*/
void inputInit()
{
	gpio_config_t io_conf;
    io_conf.pull_down_en = 0;

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;

    //bit mask of the pins, use GPIO... here
	for (int i=0; i < NELEMS(keymap[0]); i++)
    	if(i==0)
			io_conf.pin_bit_mask = (1ULL<<keymap[0][i].gpio);
		else
			io_conf.pin_bit_mask |= (1ULL<<keymap[0][i].gpio);

	io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    
#ifndef CONFIG_HW_ODROID_GO
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(GPIOEvent));
    //start gpio task
	//xTaskCreatePinnedToCore(&gpioTask, "GPIO", 1500, NULL, 7, NULL, 0);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_SHARED);

    //hook isr handler
	for (int i=0; i < NELEMS(keymap[0]); i++)
    	gpio_isr_handler_add(keymap[0][i].gpio, gpio_isr_handler, (void*) keymap[0][i].gpio);
#else
	adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ODROID_GAMEPAD_IO_X, ADC_ATTEN_11db);
	adc1_config_channel_atten(ODROID_GAMEPAD_IO_Y, ADC_ATTEN_11db);
#endif    

	printf("keyboard: GPIO task created.\n");
    initInput = true;
}


typedef struct {
	char key;
	SDL_Scancode scancode;
    SDL_Keycode keycode;
} UartGamepadKeyMap;

typedef struct {
    Uint32 type;        /**< ::SDL_KEYDOWN or ::SDL_KEYUP */
    SDL_Scancode scancode;
    SDL_Scancode keycode;
} UartKeyEvent;

#define UARTGAMEPAD_KEY_ESC             'n'
#define UARTGAMEPAD_KEY_UP              'w'
#define UARTGAMEPAD_KEY_DOWN            's'
#define UARTGAMEPAD_KEY_LEFT            'a'
#define UARTGAMEPAD_KEY_RIGHT           'd'
#define UARTGAMEPAD_KEY_FIRE            'k'
#define UARTGAMEPAD_KEY_USE             'e'
#define UARTGAMEPAD_KEY_JUMP            'm'

typedef struct {
    SDL_Scancode scancode;
    SDL_Keycode keycode;
    char mapped_uart_char;
    char key_as_text[10];
    int64_t press_time;
} keyMapElement;

static const int64_t key_unpress_delay = 350000;
#define KEY_MAX 8

keyMapElement uart_keymap[KEY_MAX] = {
    {SDL_SCANCODE_UP,       SDLK_UP,        UARTGAMEPAD_KEY_UP,         "UP",       -1},
    {SDL_SCANCODE_RIGHT,    SDLK_RIGHT,     UARTGAMEPAD_KEY_RIGHT,      "RIGHT",    -1},
    {SDL_SCANCODE_DOWN,     SDLK_DOWN,      UARTGAMEPAD_KEY_DOWN,       "DOWN",     -1},
    {SDL_SCANCODE_LEFT,     SDLK_LEFT,      UARTGAMEPAD_KEY_LEFT,       "LEFT",     -1},
    {SDL_SCANCODE_LCTRL,    SDLK_LCTRL,     UARTGAMEPAD_KEY_FIRE,       "FIRE",     -1},
    {SDL_SCANCODE_SPACE,    SDLK_SPACE,     UARTGAMEPAD_KEY_USE,        "USE",      -1},
    {SDL_SCANCODE_ESCAPE,   SDLK_ESCAPE,    UARTGAMEPAD_KEY_ESC,        "ESC",      -1},
    {SDL_SCANCODE_A,        SDLK_a,         UARTGAMEPAD_KEY_JUMP,       "JUMP",     -1},
};

void postDukeKeyEvent(int sdl_scancode, int sdl_keycode, int key_event_type) {
    UartKeyEvent ev;
    ev.scancode = sdl_scancode;
    ev.keycode = sdl_keycode;
    ev.type = key_event_type;
    xQueueSend(uart_evt_queue, &ev, NULL);
}

void keyCheckAutoUnpress() {
    int64_t curr_time = esp_timer_get_time();

    for(int i = 0 ; i < KEY_MAX ; i++) {
        if((uart_keymap[i].press_time > 0) && (uart_keymap[i].press_time + key_unpress_delay < curr_time)) {
            uart_keymap[i].press_time = -1;
            postDukeKeyEvent(uart_keymap[i].scancode, uart_keymap[i].keycode, SDL_KEYUP);
        }
    }
}

void uartGamepadProcessInput(char input) {

    for(int i = 0 ; i < KEY_MAX ; i++) {
        if(input == uart_keymap[i].mapped_uart_char) {
            uart_keymap[i].press_time = esp_timer_get_time();
            postDukeKeyEvent(uart_keymap[i].scancode, uart_keymap[i].keycode, SDL_KEYDOWN);
            ets_printf("uartGamepad: %s\n", uart_keymap[i].key_as_text);
            return;
        }
    }

    ets_printf("uartGamepad: unmapped key (%c)\n", input);
}

#define UART_MODULE_NUM    0
#define UART_BAUD_RATE     115200
#define UART_BUF_SIZE      512

void uartGamepadTask(void *arg) {
    ets_printf("uartGamepad: task starting\n");

    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART_MODULE_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_MODULE_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_MODULE_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    uint8_t *uart_rx_buf = (uint8_t*)malloc(UART_BUF_SIZE);
    int uart_buf_iter = 0;
    int uart_rx_buf_len = 0;

    uart_evt_queue = xQueueCreate(10, sizeof(UartKeyEvent));

    while(1) {
        uart_rx_buf_len = uart_read_bytes(UART_MODULE_NUM, uart_rx_buf, UART_BUF_SIZE, 20 / portTICK_RATE_MS);

        for(uart_buf_iter = 0 ; uart_buf_iter < uart_rx_buf_len ; uart_buf_iter++) {
            uartGamepadProcessInput(uart_rx_buf[uart_buf_iter]);
        }
        /* uart_write_bytes(UART_MODULE_NUM, (const char *) uart_rx_buf, uart_rx_buf_len); */
        keyCheckAutoUnpress();
    }
}

void uartGamepadInit() {
    ets_printf("uartGamepad: initializing\n");
    xTaskCreatePinnedToCore(&uartGamepadTask, "uartgamepad", 1024, NULL, 7, NULL, 0);
}
