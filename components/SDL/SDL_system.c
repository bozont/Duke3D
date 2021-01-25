#include "SDL_system.h"

struct SDL_mutex
{
    pthread_mutex_t id;
    SemaphoreHandle_t handle;
#if FAKE_RECURSIVE_MUTEX
    int recursive;
    pthread_t owner;
#endif
};

void SDL_ClearError(void)
{

}

void SDL_Delay(Uint32 ms)
{
    const TickType_t xDelay = ms / portTICK_PERIOD_MS;
    vTaskDelay( xDelay );
}

char *SDL_GetError(void)
{
    return (char *)"";
}

int SDL_Init(Uint32 flags)
{
    if(flags == SDL_INIT_VIDEO)
        SDL_InitSubSystem(flags);
    return 0;
}

void SDL_Quit(void)
{

}

void SDL_InitSD(void)
{
    printf("Initialising SD Card\n");

    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_1;
    host.max_freq_khz = 40000;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;

    SDL_LockDisplay();

    ret = esp_vfs_fat_sdmmc_mount("/sd", &host, &slot_config, &mount_config, &card);

    if(ret != ESP_OK) {
        if(ret == ESP_FAIL) {
            printf("Init_SD: Failed to mount filesystem.\n");
        } else {
           printf("Init_SD: Failed to initialize the card. %d\n", ret);
        }
        return false;
    }

    printf("Init_SD: init successful. Card info:\n");
    sdmmc_card_print_info(stdout, card);

    SDL_UnlockDisplay();

    printf("Init_SD: SD card opened.\n");
}

const SDL_version* SDL_Linked_Version()
{
    SDL_version *vers = malloc(sizeof(SDL_version));
    vers->major = SDL_MAJOR_VERSION;                 
    vers->minor = SDL_MINOR_VERSION;                 
    vers->patch = SDL_PATCHLEVEL;      
    return vers;
}

char *** allocateTwoDimenArrayOnHeapUsingMalloc(int row, int col)
{
	char ***ptr = malloc(row * sizeof(*ptr) + row * (col * sizeof **ptr) );

	int * const data = ptr + row;
	for(int i = 0; i < row; i++)
		ptr[i] = data + i * col;

	return ptr;
}

void SDL_DestroyMutex(SDL_mutex* mutex)
{

}

SDL_mutex* SDL_CreateMutex(void)
{
    SDL_mutex* mut = malloc(sizeof(SDL_mutex));
    mut->handle = xSemaphoreCreateMutex();
    //mut->handle = xSemaphoreCreateBinary();
    return mut;
}

int SDL_LockMutex(SDL_mutex* mutex)
{
    xSemaphoreTake(mutex->handle, 1000 / portTICK_RATE_MS);
    return 0;
}

int SDL_UnlockMutex(SDL_mutex* mutex)
{
    xSemaphoreGive(mutex->handle);
    return 0;
}