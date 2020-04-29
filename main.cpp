#include "mbed.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#define UINT14_MAX        16383
// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0
#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1
// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7

I2C i2c(PTD9,PTD8);
Serial pc(USBTX, USBRX);
DigitalOut bLED(LED3);
InterruptIn button(SW2);

int m_addr = FXOS8700CQ_SLAVE_ADDR1;

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
void FXOS8700CQ_writeRegs(uint8_t * data, int len);

EventQueue queue(32 * EVENTS_EVENT_SIZE);
Thread thread;
Ticker ticker;

void blink() {
    bLED = !bLED;
}

void logger() {
   Timer t10s;

   ticker.attach(&blink, 0.2);
   pc.baud(115200);
   uint8_t data[2], res[6];
   int16_t acc16;
   float s[3]; //stationary
   float d[3]; //delta
   int initial = 0;

   // Enable the FXOS8700Q
   FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &data[1], 1);
   data[1] |= 0x01;
   data[0] = FXOS8700Q_CTRL_REG1;
   FXOS8700CQ_writeRegs(data, 2);

   t10s.start();
   while (t10s.read() <= 11){
      FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

    if(initial == 0){
        acc16 = (res[0] << 6) | (res[1] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        s[0] = ((float)acc16) / 4096.0f;
        d[0] = ((float)acc16) / 4096.0f;

        acc16 = (res[2] << 6) | (res[3] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        s[1] = ((float)acc16) / 4096.0f;
        d[1] = ((float)acc16) / 4096.0f;

        acc16 = (res[4] << 6) | (res[5] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        s[2] = ((float)acc16) / 4096.0f;
        d[2] = ((float)acc16) / 4096.0f;
        initial++;
    }else{
        acc16 = (res[0] << 6) | (res[1] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        d[0] = ((float)acc16) / 4096.0f;

        acc16 = (res[2] << 6) | (res[3] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        d[1] = ((float)acc16) / 4096.0f;

        acc16 = (res[4] << 6) | (res[5] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        d[2] = ((float)acc16) / 4096.0f;
    }  

    
    float disx = 9.8*sqrt((d[1]-s[1])*(d[2]-s[1]))*0.01/2;
    float dis5 = 0.05;

    if(disx >= dis5){
        pc.printf("%f\r\n%f\r\n%f\r\n%d\r\n", d[0], d[1], d[2], 1);
    }else{
        pc.printf("%f\r\n%f\r\n%f\r\n%d\r\n", d[0], d[1], d[2], 0);
    }

    wait(0.1);
  }
   
   ticker.detach();
   t10s.stop();
}
int main(){
    bLED = 1;

    thread.start(callback(&queue, &EventQueue::dispatch_forever));
    button.rise(queue.event(logger));
}

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {
   char t = addr;
   i2c.write(m_addr, &t, 1, true);
   i2c.read(m_addr, (char *)data, len);
}

void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
   i2c.write(m_addr, (char *)data, len);
}