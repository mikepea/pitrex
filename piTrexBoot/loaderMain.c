#include <stdio.h>
#include <stdbool.h>

#include <baremetal/rpi-gpio.h>
#include <baremetal/rpi-aux.h>
#include <bcm2835_vc.h>
#include <ff.h>

#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include <vectrex/baremetalUtil.h>

#include <baremetal/vectors.h>

static FATFS fat_fs;  	/* File system object */
GlobalMemSettings **settingsPointer;
GlobalMemSettings loaderSettings;


void cleanup_before_linux (void)
{
  /*
   * this function is called just before we call linux
   * it prepares the processor for linux
   *
   * we turn off caches etc ...
   */
  mmu_disable(); //dcache_disable(); + icache_disable(); + clean_data_cache();
  cache_flush(); /* flush I/D-cache */
}

bool fileExists(TCHAR FILE_NAME[])
{
  return true;
}


#define MAX_LOAD (1024*1024*100) // 100 MB
void loadAndStart(TCHAR FILE_NAME[], char *parameter)
{
    void *progSpace = (void *) 0x8000;

    FRESULT rc_rd = FR_DISK_ERR;
    FIL file_object_rd;
    rc_rd = f_open(&file_object_rd, FILE_NAME, (unsigned char) FA_READ);

    v_noSound();

    if (rc_rd != FR_OK)
    {
      printf("Could not open file %s (%i) \r\n", FILE_NAME, rc_rd);
    }
    else
    {
      /*
        FIL* fp, 	/* Pointer to the file object
        void* buff,	/* Pointer to data buffer
        UINT btr,	/* Number of unsigned chars to read
        UINT* br	/* Pointer to number of unsigned chars read
      */
      printf("Loading: %s \r\n", FILE_NAME);
      unsigned int fsize = MAX_LOAD;

      rc_rd = f_read(&file_object_rd, progSpace, fsize, &fsize);
      if ( rc_rd!= FR_OK)
      {
        printf("File not loaded (size got = %i)\r\n", rc_rd);
        f_close(&file_object_rd);
      }
      else
      {
        f_close(&file_object_rd);
        int c=0;
        while (*parameter != (char) 0)
        {
          loaderSettings.parameter1[c++] =  (unsigned char) *parameter;
          parameter++;
          if (c==15) break;
        }
        loaderSettings.parameter1[c]= (unsigned char) 0;

        printf("Starting loaded file... \r\n");
        isb();
        dsb();
        dmb();
        cleanup_before_linux();
        // correct start registers and jump to 8000
        __asm__ __volatile__(
          "mov r5, #0x0080   \n\t"
          "ldr r0, [r5]      \n\t"
          "mov r5, #0x0084   \n\t"
          "ldr r1, [r5]      \n\t"
          "mov r5, #0x0088   \n\t"
          "ldr r2, [r5]      \n\t"
          "ldr pc, = 0x8000  \n\t"
        );
      }
    }
}


int selectedMenu;
int selectionMade;
int selectionStart;

// time to wait before booting to Linux
// TODO make this configurable via a file, default to no timeout
int bootTimeout;
int currentTicks;

bool checkTimedOut() {
  if (currentTicks > bootTimeout) return true;
  return false;
}


// menu
// todo read from disk...
void bootMenu(void)
{
  typedef struct MenuItem {
    char *DISPLAYNAME;
    char *img;
    char *param;
    bool present;
  } PiTrexMenuItem;
  PiTrexMenuItem piTrexMenuItems[] = {
    {"RASPBIAN", "kernel.img", "", false},
    {"CALIBRATE", "calibrate.img", "", false},
    {"ASTEROIDS", "asteroids_sbt.img", "", false},
    {"TAILGUNNER", "tailgunner.img", "", false},
    {"VECX EMUL", "vecxemul.img", "", false},
    {"VECX DIRECT", "vecxemuld.img", "", false},
    {"GYROCKS", "gyrocks.img", "", false},
    {"HELLO", "hello.img", "", false},
    {"PACMAN", "pacman.img", "", false},
    {"GRAVITAR", "gravitar.img", "", false},
    {"SPACEDUEL", "spaceduel.img", "", false},
    {"TEMPEST", "tempest.img", "", false},
    {"LUNAR LANDER", "lunar.img", "", false},
    {"BLACK WIDOW", "blackwidow.img", "", false},
    {"DELUXE", "deluxe.img", "", false},
    {"RED BARON", "redbaron.img", "", false},
    {"RIPOFF", "cine.img", "ripoff", false},
    {"SPACE WARS", "cine.img", "spacewars", false},
    {"SPEED FREAK", "cine.img", "speedfreak", false},
    {"STAR HAWK", "cine.img", "starhawk", false},
    {"STAR CASTLE", "cine.img", "starcastle", false},
    {"DEMON", "cine.img", "demon", false},
    {"ARMOR A", "cine.img", "armorattack", false},
    {"SUNDANCE", "cine.img", "sundance", false},
    {"BARRIER", "cine.img", "barrier", false},
    {"BOXING BUGS", "cine.img", "boxingbugs", false},
    {"COSMIC CHASM", "cine.img", "cosmicchasm", false},
    {"SOLAR QUEST", "cine.img", "solarquest", false},
    {"TAILGUNNER", "cine.img", "tailgunner", false},
    {"WAR OF THE WORLDS", "cine.img", "waroftheworlds", false},
    {"WARRIOR", "cine.img", "warrior", false},
    {"BATTLE ZONE", "battlezone.img", "", false},
  };

  char *selected = ">";
  int max = (sizeof(piTrexMenuItems)/sizeof(piTrexMenuItems[0]))-1;

  char *selectedCursor = ">";

  int x = -50;
  int SPREAD = 15;
  int brightnesses[]={40,50,70,90,70,50,40};
  int brightnessCounter=0;
  for (int y=20; y<20+7*SPREAD;y+=SPREAD)
  {
    int currentItem = brightnessCounter + selectionStart;
    if (!(currentItem<0) && (!(currentItem>max)))
    {
      int b;
      if (piTrexMenuItems[currentItem].present)
        b = brightnesses[brightnessCounter];
      else
        b = 30; // unavailable, greyed out

      v_printString(x,y, piTrexMenuItems[currentItem].DISPLAYNAME, 5, b);
    }
    if (brightnesses[brightnessCounter] == 90) selectedMenu = currentItem;
    brightnessCounter++;
  }

  v_printString(-60, 20+3*SPREAD, selectedCursor, 5, 90);

  if ((currentJoy1Y>50) && (selectionMade==0))
  {
    selectionStart++;
    if (selectionStart>max-3)
    {
      selectionStart=max-3;
    }
    selectionMade = 1;
  }
  if ((currentJoy1Y<-50) && (selectionMade==0))
  {
    selectionStart--;
    if (selectionStart<0-3) selectionStart=0-3;
    selectionMade = 1;
  }
  if (ABS(currentJoy1Y)<20) selectionMade =0;

  if ((currentButtonState&0x0f) == (0x04)) // exactly button 3
  {
    int i;
    loaderSettings.lastSelection = (unsigned char) selectedMenu; // remember last started game

    for (i = 0; i <= max; i++) {
      if (selectedMenu==i) loadAndStart(piTrexMenuItems[i].img, piTrexMenuItems[i].param);
    }
  }

  // boot to Rasbian if we time out
  if (checkTimedOut) loadAndStart(piTrexMenuItems[0].img, piTrexMenuItems[0].param);

}


#include "ymStuff.i"
#include "playRaw.i"

extern int __bss_start__;
extern int __bss_end__;


/** Main function - we'll never return from here */
void loaderMain()
{
    printf("Loader starting...\r\n");
    printf("BSS start: %X, end: %X\r\n", &__bss_start__, &__bss_end__);

    printf("SettingPointer: %08x, settings: %0x08", settingsPointer, &loaderSettings);

    settingsPointer = (GlobalMemSettings **)0x0000008c;
    *settingsPointer = &loaderSettings;

    tweakVectors();
    printf("Start mounting fs...\r\n");
    FRESULT result = f_mount(&fat_fs, (const TCHAR *) "0:", (unsigned char) 1);
    if (result != FR_OK)
    {
      vectrexinit(1); // pitrex
      v_init(); // vectrex interface
      printf("NO filesystem...!\r\n");
      printf("f_mount failed! %d\r\n", (int) result);
      v_error("MOUNT FAILED");
    }
    else
    {
      printf("FAT filesystem found!\r\n");
    }
    selectionStart = 0;
    selectionMade = 0;
    bootTimeout = 10000000;

    vectrexinit(1); // pitrex
    v_init(); // vectrex interface
    loaderSettings.loader = loaderMain;
    selectedMenu = loaderSettings.lastSelection;
    selectionStart = selectedMenu-3;

    loadAndPlayRAW();
    v_init(); // vectrex interface
    usePipeline = 1;

    int ymloaded = loadYM();

    int s = 0;
    int bDir = 1;
    int b = 30;
//    char *ss[] = {"PITREX","BY","KEVIN","GRAHAM","MALBAN"};
    char *ss[] = {"PITREX"};
    while(1)
    {
        v_WaitRecal();
        v_doSound();
        v_readButtons();
        v_readJoystick1Analog();
        b=b+bDir;
        if (b==70) bDir = -1;
        if (b==20)
        {
          bDir = 1;
          s++;
          if (s==5) s=0;
        }

        v_printString(-20, -100, ss[s], 5, b);
        bootMenu();
        if (ymloaded) v_playYM();
        currentTicks++;
    }
}
