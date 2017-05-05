#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SIZE	(1*1024*1024)

int menuTop = 8, statusTop = 15;

//---------------------------------------------------------------------------------
int saveToFile(const char *filename, u8 *buffer, size_t size) {
//---------------------------------------------------------------------------------
	FILE *f = fopen(filename, "wb");
	if (NULL==f) return -1;
	size_t written = fwrite(buffer, 1, size, f);
	fclose(f);
	if (written != size) return -2;
	return 0;
}

//---------------------------------------------------------------------------------
int readJEDEC() {
//---------------------------------------------------------------------------------

	fifoSendValue32(FIFO_USER_01, 1);

	fifoWaitValue32(FIFO_USER_01);

	return  fifoGetValue32(FIFO_USER_01);
}

struct menuItem {
	const char* name;
	fp function;
};

u8 *firmware_buffer;
size_t userSettingsOffset, fwSize, wifiOffset, wifiSize;

//---------------------------------------------------------------------------------
void clearStatus() {
//---------------------------------------------------------------------------------
	iprintf("\x1b[%d;0H\x1b[J\x1b[15;0H",statusTop);
}

//---------------------------------------------------------------------------------
void dummy() {
//---------------------------------------------------------------------------------
	clearStatus();
	iprintf("\x1b[%d;6HNOT IMPLEMENTED!",statusTop+3);
}

char dirname[15] = "FW";
char serial[13];

//---------------------------------------------------------------------------------
void backupFirmware() {
//---------------------------------------------------------------------------------

	clearStatus();

	readFirmware(0, firmware_buffer, fwSize);

	if (saveToFile("firmware.bin", firmware_buffer, fwSize) < 0) {
		iprintf("Error saving firmware!\n");
	} else {
		iprintf("Firmware saved as\n\n%s/firmware.bin", dirname );
	}
}

//---------------------------------------------------------------------------------
void backupBIOS() {
//---------------------------------------------------------------------------------
	int dumpcmd = 0;

	clearStatus();

	const char *arm7file, *arm9file;
	size_t arm7size, arm9size;

	if (isDSiMode()) {
		arm7file = "bios7i.bin";
		arm7size = 64 * 1024;
		arm9file = "bios9i.bin";
		arm9size = 64 * 1024;
		dumpcmd = 3;
	} else {
		arm7file = "bios7.bin";
		arm7size = 16 * 1024;
		arm9file = "bios9.bin";
		arm9size = 32 * 1024;
		dumpcmd = 2;
	}

	if (saveToFile(arm9file, (u8*)0xffff0000, arm9size ) < 0) {
		iprintf("Error saving arm9 bios\n");
		return;
	}

	fifoSendValue32(FIFO_USER_01, dumpcmd);
	fifoSendValue32(FIFO_USER_01, (u32)firmware_buffer);

	fifoWaitValue32(FIFO_USER_01);

	fifoGetValue32(FIFO_USER_01);

	if (saveToFile(arm7file, firmware_buffer, arm7size) < 0 ) {
		iprintf("Error saving arm7 bios\n");
		return;
	}

	iprintf("BIOS saved as\n\n%1$s/%2$s\n%1$s/%3$s", dirname, arm7file, arm9file );

}

//---------------------------------------------------------------------------------
void backupSettings() {
//---------------------------------------------------------------------------------

	clearStatus();

	readFirmware(userSettingsOffset, firmware_buffer + userSettingsOffset, 512);

	if (saveToFile("UserSettings.bin", firmware_buffer + userSettingsOffset, 512) < 0) {
		iprintf("Error saving settings1!\n");
	} else {
		iprintf("User settings saved as\n\n%s/UserSettings.bin", dirname );
	}
}

//---------------------------------------------------------------------------------
void backupWifi() {
//---------------------------------------------------------------------------------

	clearStatus();

	readFirmware(wifiOffset, firmware_buffer + wifiOffset, wifiSize);

	if (saveToFile("WifiSettings.bin", firmware_buffer + wifiOffset, wifiSize) < 0) {
		iprintf("Error saving Wifi settings!\n");
	} else {
		iprintf("Wifi settings saved as\n\n%s/WifiSettings.bin", dirname );
	}
}

//---------------------------------------------------------------------------------
void backupNAND() {
//---------------------------------------------------------------------------------

	clearStatus();


	if (!isDSiMode()) {
		iprintf("Not a DSi!\n");
	} else {

		FILE *f = fopen("nand.bin", "wb");

		if (NULL == f) {
			iprintf("failure creating nand.bin\n");
		} else {
			iprintf("Writing %s/nand.bin\n\n", dirname );
			size_t i;
			size_t sectors = 128;
			size_t blocks = nand_GetSize() / sectors;
			for (i=0; i < blocks; i++) {
				if(!nand_ReadSectors(i * sectors,sectors,firmware_buffer)) {
					iprintf("\nError reading NAND!\n");
					break;
				}
				size_t written = fwrite(firmware_buffer, 1, 512 * sectors, f);
				if(written != 512 * sectors) {
					iprintf("\nError writing to SD!\n");
					break;
				}
				iprintf("Block %d of %d\r", i+1, blocks);
			}
			fclose(f);
		}
	}

}

bool quitting = false;

//---------------------------------------------------------------------------------
void quit() {
//---------------------------------------------------------------------------------
	quitting = true;
}

struct menuItem mainMenu[] = {
	{ "Backup Firmware", backupFirmware } ,
	{ "Dump Bios", backupBIOS } ,
	{ "Backup User Settings", backupSettings } ,
	{ "Backup Wifi Settings", backupWifi } ,
	{ "Backup DSi NAND", backupNAND},
/*
	TODO

	{ "Restore Firmware", dummy } ,
	{ "Restore User Settings", dummy } ,
	{ "Restore Wifi Settings", dummy } ,
*/	{ "Exit", quit }
};

//---------------------------------------------------------------------------------
void showMenu(menuItem menu[], int count) {
//---------------------------------------------------------------------------------
	int i;
	for (i=0; i<count; i++ ) {
		iprintf("\x1b[%d;5H%s", i + menuTop, menu[i].name);
	}
}


//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
	defaultExceptionHandler();

	consoleDemoInit();

	if (!fatInitDefault()) {
		printf("FAT init failed!\n");
	} else {

		iprintf("DS(i) firmware tool %s\n",VERSION);

		firmware_buffer = (u8 *)memalign(32,MAX_SIZE);

		readFirmware(0, firmware_buffer, 512);

		iprintf("\x1b[3;0HMAC ");
		for (int i=0; i<6; i++) {
			printf("%02X", firmware_buffer[0x36+i]);
			sprintf(&dirname[2+(2*i)],"%02X",firmware_buffer[0x36+i]);
			if (i < 5) printf(":");
		}


		dirname[14] = 0;

		mkdir(dirname, 0777);
		chdir(dirname);

		userSettingsOffset = (firmware_buffer[32] + (firmware_buffer[33] << 8)) *8;

		fwSize = userSettingsOffset + 512;

		iprintf("\n%dK flash, jedec %X\n", fwSize/1024,readJEDEC());

		iprintf("NAND size %d sectors\n",nand_GetSize());

		wifiOffset = userSettingsOffset - 1024;
		wifiSize = 1024;

		if ( firmware_buffer[29] == 0x57 ) {
			wifiOffset -= 1536;
			wifiSize += 1536;
		}

		int count = sizeof(mainMenu) / sizeof(menuItem);

		showMenu(mainMenu, count);

		int selected = 0;
		quitting = false;

		while(!quitting) {
				iprintf("\x1b[%d;3H]\x1b[23C[",selected + menuTop);
				swiWaitForVBlank();
				scanKeys();
				int keys = keysDownRepeat();
				iprintf("\x1b[%d;3H \x1b[23C ",selected + menuTop);
				if ( (keys & KEY_UP)) selected--;
				if (selected < 0)	selected = count - 1;
				if ( (keys & KEY_DOWN)) selected++;
				if (selected == count)	selected = 0;
				if ( keys & KEY_A ) mainMenu[selected].function();
		}
	}

	return 0;
}
