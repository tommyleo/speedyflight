#ifndef TELEMETRY_COMMON_H_
#define TELEMETRY_COMMON_H_

void initTelemetry(USART_TypeDef *USARTx);

void checkTelemetryState(void);

void handleTelemetry(void);

#endif /* TELEMETRY_COMMON_H_ */
