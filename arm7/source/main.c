#include <nds.h>

#include <nds.h>
#include <dswifi7.h>
#include <maxmod7.h>

//---------------------------------------------------------------------------------
void VblankHandler(void) {
//---------------------------------------------------------------------------------
	Wifi_Update();
}


//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
	inputGetAndSend();
}

volatile bool exitflag = false;

//---------------------------------------------------------------------------------
void powerButtonCB() {
//---------------------------------------------------------------------------------
	exitflag = true;
}

static u8 readwriteSPI(u8 data) {
	REG_SPIDATA = data;
	SerialWaitBusy();
	return REG_SPIDATA;
}

void readBios(u8 *buffer);

//---------------------------------------------------------------------------------
void dumpBios() {
//---------------------------------------------------------------------------------

	while(!fifoCheckValue32(FIFO_USER_01)) {
		swiIntrWait(1,IRQ_FIFO_NOT_EMPTY);
	}


	u8 *dumped_bios = (u8 *)fifoGetValue32(FIFO_USER_01);

	readBios(dumped_bios);

	fifoSendValue32(FIFO_USER_01,0);

}


//---------------------------------------------------------------------------------
int readJEDEC() {
//---------------------------------------------------------------------------------
	// read jedec id
	int jedec = 0;
	REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_NVRAM | SPI_CONTINUOUS;
	readwriteSPI(SPI_EEPROM_RDID);
	jedec = readwriteSPI(0);
	jedec = ( jedec << 8 ) + readwriteSPI(0);
	jedec = ( jedec << 8 ) + readwriteSPI(0);
	
	REG_SPICNT = 0;

	return jedec;

}

//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
	readUserSettings();

	irqInit();
	// Start the RTC tracking IRQ
	initClockIRQ();
	fifoInit();

	mmInstall(FIFO_MAXMOD);

	SetYtrigger(80);

	installWifiFIFO();
	installSoundFIFO();

	installSystemFIFO();

	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_VBLANK, VblankHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);
	
	setPowerButtonCB(powerButtonCB);   

	// Keep the ARM7 mostly idle
	while (!exitflag) {

		swiIntrWait(1,IRQ_FIFO_NOT_EMPTY);

		if (fifoCheckValue32(FIFO_USER_01)) {

			int command = fifoGetValue32(FIFO_USER_01);

			if (command == 1) fifoSendValue32(FIFO_USER_01,readJEDEC());
			if (command == 2) dumpBios();
		}

		if ( 0 == (REG_KEYINPUT & (KEY_SELECT | KEY_START | KEY_L | KEY_R))) {
			exitflag = true;
		}
	}
	return 0;
}
