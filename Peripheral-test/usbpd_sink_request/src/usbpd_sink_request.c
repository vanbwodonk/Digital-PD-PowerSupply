/*
 * Example USB-PD Sink — simple voltage cycler
 */
#include "ch32fun.h"
#include "usbcdc_internal.h"

#define USBPD_IMPLEMENTATION
#include "usbpd.h"

#define LOG(fmt, ...)                                                                    \
  do {                                                                                   \
    char _log_buf[128];                                                                  \
    int _log_len = mini_snprintf(_log_buf, sizeof(_log_buf), fmt "\r\n", ##__VA_ARGS__); \
    if (_log_len > (int)sizeof(_log_buf)) _log_len = sizeof(_log_buf);                   \
    if (_log_len > 0) CDC_write_buf(_log_buf, _log_len);                                 \
  } while (0)

bool USBPD_RequestVoltage(uint32_t target_mV) {
  USBPD_SPR_CapabilitiesMessage_t* caps;
  const size_t count = USBPD_GetCapabilities(&caps);

  for (size_t i = 0; i < count; i++) {
    const USBPD_SourcePDO_t* pdo = &caps->Source[i];
    switch (pdo->Header.PDOType) {
      case eUSBPD_PDO_FIXED:
        if (pdo->FixedSupply.VoltageIn50mV * 50 == target_mV)
          return USBPD_SelectPDO(i, 0) == eUSBPD_OK;
        break;
      case eUSBPD_PDO_VARIABLE:
        if (target_mV >= pdo->VariableSupply.MinVoltageIn50mV * 50 &&
            target_mV <= pdo->VariableSupply.MaxVoltageIn50mV * 50)
          return USBPD_SelectPDO(i, 0) == eUSBPD_OK;
        break;
      default:
        break;
    }
  }
  return false;
}

bool USBPD_RequestPPSVoltage(uint32_t target_mV) {
  USBPD_SPR_CapabilitiesMessage_t* caps;
  const size_t count = USBPD_GetCapabilities(&caps);

  for (size_t i = 0; i < count; i++) {
    const USBPD_SourcePDO_t* pdo = &caps->Source[i];
    if (USBPD_IsPPS(pdo)) {
      const uint32_t minV = pdo->SPR_PPS.MinVoltageIn100mV;
      const uint32_t maxV = pdo->SPR_PPS.MaxVoltageIn100mV;
      const uint32_t voltageIn100mV = target_mV / 100;
      if (voltageIn100mV >= minV && voltageIn100mV <= maxV)
        return USBPD_SelectPDO(i, voltageIn100mV) == eUSBPD_OK;
    }
  }
  return false;
}

int main(void) {
  SystemInit();

  CDC_init();
  while (!CDC_connected());

  RCC->APB2PCENR |= (RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA);

  USBPD_Result_e result = USBPD_Init(eUSBPD_VCC_5V0);
  if (eUSBPD_OK != result) {
    LOG("USB PD init failed: %s", USBPD_ResultToStr(result));
    while (1);
  }

  LOG("Negotiating USB PD...");
  while (eUSBPD_BUSY == (result = USBPD_SinkNegotiate()));

  if (eUSBPD_OK != result) {
    LOG("Negotiation failed: %s", USBPD_ResultToStr(result));
    while (1);
  }

  LOG("PD negotiated, cycling voltages every 5s");

  const uint32_t voltages[] = {5000, 9000, 12000, 15000, 20000};
  const char* names[] = {"5V", "9V", "12V", "15V", "20V"};

  while (1) {
    for (int i = 0; i < 5; i++) {
      if (USBPD_RequestVoltage(voltages[i]))
        LOG("%s", names[i]);
      else
        LOG("%s not supported", names[i]);
      Delay_Ms(5000);
    }
  }
}
