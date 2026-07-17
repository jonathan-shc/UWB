/* ESP-IDF GPIO/IRQ backend for the DW3000 decadriver — implements dw3000_hw.h.
 * Replaces the Zephyr deps/dw3000/platform/dw3000_hw.c (not compiled here).
 *
 * IRQ mirrors the Zephyr design: the GPIO ISR wakes a dedicated high-priority
 * task (pinned to core 1) that calls dwt_isr() while the IRQ line stays high —
 * dwt_isr does SPI, so it cannot run in true ISR context. Also provides the
 * cycle-counter diag symbols that dwt_uwb_driver/dw3000/dw3000_device.c
 * references (Xtensa CCOUNT via esp_cpu_get_cycle_count). */
#include "dw3000_hw.h"

#include "board_pins.h"
#include "deca_device_api.h"
#include "dw3000_spi.h"
#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *const TAG = "dw3000_hw";

/* Diag cycle-counter exports referenced by dw3000_device.c (RF-arm trace). */
volatile uint32_t g_dw_cyc_gpio;
volatile uint32_t g_dw_cyc_work;
volatile uint32_t g_dw_cyc_isrdone;
volatile uint32_t g_dw_cyc_per_us = 240; /* ESP32-S3 core @240 MHz */

uint32_t dw3000_dwt_cyccnt(void) { return esp_cpu_get_cycle_count(); }

static bool s_asleep;
static bool s_irq_enabled;
static SemaphoreHandle_t s_irq_sem;
static TaskHandle_t s_irq_task;

void dw3000_hw_mark_asleep(void) { s_asleep = true; }
bool dw3000_hw_is_asleep(void) { return s_asleep; }

int dw3000_hw_init(void)
{
	/* Reset line: input = released (external pull-up); active low when driven. */
	gpio_config_t rst = {
		.pin_bit_mask = 1ULL << WOZ_DW3000_PIN_RST,
		.mode = GPIO_MODE_INPUT,
	};
	gpio_config(&rst);

	/* Wakeup line held active (matches the Zephyr GPIO_OUTPUT_ACTIVE default). */
	gpio_config_t wk = {
		.pin_bit_mask = 1ULL << WOZ_DW3000_PIN_WAKEUP,
		.mode = GPIO_MODE_OUTPUT,
	};
	gpio_config(&wk);
	gpio_set_level(WOZ_DW3000_PIN_WAKEUP, 1);

	return dw3000_spi_init();
}

static void IRAM_ATTR dw3000_gpio_isr(void *arg)
{
	(void)arg;
	g_dw_cyc_gpio = esp_cpu_get_cycle_count();
	BaseType_t hpw = pdFALSE;
	xSemaphoreGiveFromISR(s_irq_sem, &hpw);
	if (hpw) {
		portYIELD_FROM_ISR();
	}
}

static void dw3000_isr_task(void *arg)
{
	(void)arg;
	for (;;) {
		xSemaphoreTake(s_irq_sem, portMAX_DELAY);
		g_dw_cyc_work = esp_cpu_get_cycle_count();
		while (gpio_get_level(WOZ_DW3000_PIN_IRQ)) {
			dwt_isr();
		}
		g_dw_cyc_isrdone = esp_cpu_get_cycle_count();
	}
}

int dw3000_hw_init_interrupt(void)
{
	static bool isr_service_installed;

	if (s_irq_sem == NULL) {
		s_irq_sem = xSemaphoreCreateBinary();
	}

	gpio_config_t irq = {
		.pin_bit_mask = 1ULL << WOZ_DW3000_PIN_IRQ,
		.mode = GPIO_MODE_INPUT,
		.intr_type = GPIO_INTR_POSEDGE,
	};
	gpio_config(&irq);

	if (!isr_service_installed) {
		esp_err_t e = gpio_install_isr_service(0);
		if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
			ESP_LOGE(TAG, "gpio_install_isr_service failed: %d", e);
			return -1;
		}
		isr_service_installed = true;
	}
	gpio_isr_handler_add(WOZ_DW3000_PIN_IRQ, dw3000_gpio_isr, NULL);

	if (s_irq_task == NULL) {
		/* High priority, pinned to core 1 (BLE/Wi-Fi live on core 0). */
		xTaskCreatePinnedToCore(dw3000_isr_task, "dw3000_isr", 4096, NULL,
					20, &s_irq_task, 1);
	}
	s_irq_enabled = true;
	ESP_LOGI(TAG, "IRQ on GPIO%d", WOZ_DW3000_PIN_IRQ);
	return 0;
}

void dw3000_hw_interrupt_enable(void)
{
	if (!s_irq_enabled) {
		gpio_intr_enable(WOZ_DW3000_PIN_IRQ);
		s_irq_enabled = true;
	}
}

void dw3000_hw_interrupt_disable(void)
{
	if (s_irq_enabled) {
		gpio_intr_disable(WOZ_DW3000_PIN_IRQ);
		s_irq_enabled = false;
	}
}

bool dw3000_hw_interrupt_is_enabled(void) { return s_irq_enabled; }

void dw3000_hw_reset(void)
{
	/* Drive reset low (assert), release to hi-Z, let the chip climb to IDLE_RC. */
	gpio_set_direction(WOZ_DW3000_PIN_RST, GPIO_MODE_OUTPUT);
	gpio_set_level(WOZ_DW3000_PIN_RST, 0);
	vTaskDelay(pdMS_TO_TICKS(1));
	gpio_set_direction(WOZ_DW3000_PIN_RST, GPIO_MODE_INPUT);
	vTaskDelay(pdMS_TO_TICKS(2));
	s_asleep = false;
}

void dw3000_hw_wakeup(void)
{
	if (!s_asleep) {
		return;
	}
	ESP_LOGI(TAG, "WAKEUP CS");
	dw3000_spi_wakeup();
	esp_rom_delay_us(2000); /* INIT_RC -> IDLE_RC startup */
	s_asleep = false;

	int spins = 0;
	while (!dwt_checkidlerc() && spins < 500) {
		esp_rom_delay_us(10);
		spins++;
	}
	if (spins >= 500) {
		ESP_LOGE(TAG, "WAKEUP: chip never reached IDLE_RC");
	}
}

void dw3000_hw_wakeup_pin_low(void)
{
	gpio_set_level(WOZ_DW3000_PIN_WAKEUP, 0);
}

void dw3000_hw_fini(void)
{
	if (s_irq_enabled) {
		gpio_intr_disable(WOZ_DW3000_PIN_IRQ);
		s_irq_enabled = false;
	}
	dw3000_spi_fini();
}
