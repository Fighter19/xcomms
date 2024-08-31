// generated by Fast Light User Interface Designer (fluid) version 1.0105

#include "xcomms.h"
#include "xcomms_data.h"
#include "options.h"

#include "log.h"
#include "config.h"
#include "xboo.h"
#include "mbv2.h"
#include "console.h"

#include <FL/fl_ask.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <string>

#ifdef __WIN32__
	#define		WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <direct.h>
#else
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/io.h>
	#include <sys/ioctl.h>
	#include <linux/ppdev.h>

#endif

std::string path;
std::string ConfigFile;


//-----------------------------------------------------------------------------
class Fl_Xcomms_Window : public Fl_Double_Window {
//-----------------------------------------------------------------------------
public:
//-----------------------------------------------------------------------------
	Fl_Xcomms_Window(int w, int h, const char *l=0) : Fl_Double_Window(w,h,l) {}

	int handle(int event)
	{
		switch (event) {
			case FL_FOCUS: return(1);
			case FL_UNFOCUS: return(1);
			case FL_KEYBOARD:
				console_keypress(Fl::event_text());
				return(1);

			default:
				return(Fl_Double_Window::handle(event));
		}
	}
};

Fl_Xcomms_Window* xcomms;

//-----------------------------------------------------------------------------
void quit(Fl_Button*, void*)
//-----------------------------------------------------------------------------
{
	XcommsCfg.x = xcomms->x();
	XcommsCfg.x = xcomms->y();
	XcommsCfg.width = xcomms->w();
	XcommsCfg.height = xcomms->h();

	WriteConfigFile(ConfigFile.c_str());

#ifndef __WIN32__
	ioperm(0x278, 4, 0);
	ioperm(0x378, 4, 0);
#endif

	exit(0);
}

char GBArom[256*1024];

typedef struct
{
	unsigned long	start_code;			// B instruction
	unsigned char	logo[0xA0-0x04];	// logo data
	char			title[0xC];			// game title name
	unsigned long	game_code;			//
	unsigned short	maker_code;			//
	unsigned char	fixed;				// 0x96
	unsigned char	unit_code;			// 0x00
	unsigned char	device_type;		// 0x80
	unsigned char	unused[7];			//
	unsigned char	game_version;		// 0x00
	unsigned char	complement;			// 800000A0..800000BC
	unsigned short	checksum;			// 0x0000
} Header;

const Header good_header =
{
	// start_code
	0xEA00002E,
	// logo
	{ 0x24,0xFF,0xAE,0x51,0x69,0x9A,0xA2,0x21,0x3D,0x84,0x82,0x0A,0x84,0xE4,0x09,0xAD,
	0x11,0x24,0x8B,0x98,0xC0,0x81,0x7F,0x21,0xA3,0x52,0xBE,0x19,0x93,0x09,0xCE,0x20,
	0x10,0x46,0x4A,0x4A,0xF8,0x27,0x31,0xEC,0x58,0xC7,0xE8,0x33,0x82,0xE3,0xCE,0xBF,
	0x85,0xF4,0xDF,0x94,0xCE,0x4B,0x09,0xC1,0x94,0x56,0x8A,0xC0,0x13,0x72,0xA7,0xFC,
	0x9F,0x84,0x4D,0x73,0xA3,0xCA,0x9A,0x61,0x58,0x97,0xA3,0x27,0xFC,0x03,0x98,0x76,
	0x23,0x1D,0xC7,0x61,0x03,0x04,0xAE,0x56,0xBF,0x38,0x84,0x00,0x40,0xA7,0x0E,0xFD,
	0xFF,0x52,0xFE,0x03,0x6F,0x95,0x30,0xF1,0x97,0xFB,0xC0,0x85,0x60,0xD6,0x80,0x25,
	0xA9,0x63,0xBE,0x03,0x01,0x4E,0x38,0xE2,0xF9,0xA2,0x34,0xFF,0xBB,0x3E,0x03,0x44,
	0x78,0x00,0x90,0xCB,0x88,0x11,0x3A,0x94,0x65,0xC0,0x7C,0x63,0x87,0xF0,0x3C,0xAF,
	0xD6,0x25,0xE4,0x8B,0x38,0x0A,0xAC,0x72,0x21,0xD4,0xF8,0x07 } ,
	// title
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
	// game code
	0x00000000,
	// maker code
	0x3130,
	// fixed
	0x96,
	// unit_code
	0x00,
	// device type
	0x80,
	// unused
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
	// game version
	0x00,
	// complement
	0x00,
	// checksum
	0x0000
};


//-----------------------------------------------------------------------------
void SendToGBA( const char *file)
//-----------------------------------------------------------------------------
{
	FILE *rom = fopen(file,"rb");
	chdir(path.c_str());

	if	(rom)
	{
		ROMimage image;
		fseek(rom,0,SEEK_END);
		image.length = ftell(rom);
		
		if ( image.length > 256 * 1024) {

			fl_alert("Rom image is too large!\nMultiboot images must be 262144 bytes or less.");
			//Log("File is too large! It must be 262144 bytes or less.\n");
			return;
		}

		fseek(rom,0,SEEK_SET);
		fread( GBArom, image.length, 1, rom);
		fclose(rom);
		image.GBArom = (unsigned short *)GBArom;


		if ( XcommsCfg.headerfix ) {

			Header* romheader = (Header *)GBArom;

			memcpy(romheader->logo, good_header.logo, sizeof(good_header.logo));

			romheader->fixed = good_header.fixed;
			romheader->device_type = good_header.device_type;
			romheader->checksum = 0;

			int n;
			char c = 0;
			char *p = (char *)romheader + 0xA0;
			for (n=0; n<0xBD-0xA0; n++) {
				c += *p++;
				}
			romheader->complement =  -(0x19+c);


		}
		NormalBoot(&image);
	}
	else
		Log("File error!\n");
}

char *separator;

//-----------------------------------------------------------------------------
void MBSend(Fl_Button*, void*)
//-----------------------------------------------------------------------------
{
	Fl_Native_File_Chooser *f = new Fl_Native_File_Chooser();
	f->type(Fl_Native_File_Chooser::BROWSE_FILE);
	f->title("Select GBA multiboot image");
	f->directory(path.c_str());
	f->filter("*.{gba,mb,bin}");


	std::string file;
	int result = f->show();
	if ( result == 0 ) file = f->filename();

	delete f;

	if	(result == 0 )	SendToGBA(file.c_str());


}

//-----------------------------------------------------------------------------
void TestForMBV2(Fl_Button*, void*) {
//-----------------------------------------------------------------------------
	DetectMBV2();
}


//-----------------------------------------------------------------------------
static Fl_RGB_Image image_send_mb(idata_send_mb, 32, 32, 4, 0);
static Fl_RGB_Image image_reset(idata_reset, 32, 32, 4, 0);
static Fl_RGB_Image image_options(idata_options, 32, 32, 4, 0);

#ifdef __WIN32__
static volatile BOOL bPrivException = FALSE;


LONG WINAPI HandlerExceptionFilter ( EXCEPTION_POINTERS *pExPtrs )
{

	if (pExPtrs->ExceptionRecord->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION)
	{
		pExPtrs->ContextRecord->Eip ++; // Skip the OUT or IN instruction that caused the exception
		Log("Exception");
		bPrivException = 1;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	else
		return EXCEPTION_CONTINUE_SEARCH;
}
#endif


//-----------------------------------------------------------------------------
int main(int argc, char **argv)
//-----------------------------------------------------------------------------
{

#ifdef __WIN32__
	HANDLE hUserPort;

  hUserPort = CreateFile("\\\\.\\UserPort", GENERIC_READ, 0, NULL,OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  CloseHandle(hUserPort); // Activate the driver
	Sleep(100); // We must make a process switch

	LPTOP_LEVEL_EXCEPTION_FILTER OldHandler = SetUnhandledExceptionFilter(HandlerExceptionFilter);
	bPrivException = FALSE;
#endif
	{
		Fl_Xcomms_Window* o = new Fl_Xcomms_Window(312, 284, "Xboo Communicator");
		o->size_range(256,256);
		xcomms = o;
    	{
    		Fl_Text_Display* o = LogWindow = new Fl_Text_Display(1, 52, 310, 232);
      		Fl_Group::current()->resizable(o);
    	}

    	{	Fl_Pack* o = new Fl_Pack(1, 3, 312, 48);
      		o->type(1);
			{	Fl_Button* o = new Fl_Button(1, 3, 48, 48);
        		o->tooltip("Send GBA Multiboot image");
        		o->image(image_send_mb);
        		o->callback((Fl_Callback*)MBSend);
      		}
      		{	Fl_Button* o = new Fl_Button(50, 3, 48, 48);
        		o->tooltip("Reset GBA");
        		o->image(image_reset);
        		o->callback((Fl_Callback*)ResetGBA);
      		}
      		{	Fl_Button* o = new Fl_Button(100, 3, 48, 48);
        		o->tooltip("Options");
        		o->image(image_options);
        		o->callback((Fl_Callback*)Options);
      		}
/*      		{	Fl_Button* o = new Fl_Button(150, 3, 48, 48);
        		o->tooltip("detect MBV2");
        		o->callback((Fl_Callback*)TestForMBV2);
      		}
*/      		o->end();
    	}
    	o->end();
  	}
	LogWindow->buffer(new Fl_Text_Buffer);
	LogWindow->buffer()->add_modify_callback(LogChanged_cb, xcomms);


	Log("Xboo Communicator "VERSION" - %s\n", __DATE__);
	#ifdef __WIN32__
		separator = "\\";

	#else
		separator = "/";
		if ( ioperm(0x278, 4, 1) || ioperm(0x378, 4, 1) ) {
			fl_alert("Xboo Conmmunicator requires root privileges\nfor raw parallel port access");
			exit(0);
		}

	#endif

#ifdef __MINGW32__
	ConfigFile = argv[0];
	ConfigFile.erase(ConfigFile.rfind(separator) + 1); // find start path

	path = ConfigFile;
	ConfigFile += "xcomms.cfg";
#else
	ConfigFile = getenv("HOME");
	ConfigFile += "/.xcomms";
#endif

	ReadConfigFile(ConfigFile.c_str());

	if ( XcommsCfg.x == 0 && XcommsCfg.y == 0) {
		XcommsCfg.x = xcomms->x();
		XcommsCfg.y = xcomms->y();
	}

	xcomms->resize(XcommsCfg.x, XcommsCfg.x, XcommsCfg.width, XcommsCfg.height);
	xcomms->show(1, argv);
	xcomms->callback((Fl_Callback*)quit);

	XbooInit();
#ifdef __WIN32__
	if (bPrivException) {
    MessageBox( NULL,	"Privileged instruction exception has occurred!\r\n\r\n"
    									"To use this program under Windows NT or Windows 2000\r\n"
    									"you need to install the driver 'UserPort.SYS' and grant\r\n"
    									"access to the ports used by this program.",
    					NULL,MB_OK);
		exit(0);
	}
	SetUnhandledExceptionFilter(OldHandler);
#endif

	std::string file;
	
 	if (argc >=2) {
		file = argv[1];
	} else {
		file = argv[0];
	}
  
	char buffer[1024];
	getcwd( buffer, 1024 );
 
	path = file;
  
	if (std::string::npos != path.rfind("\\")) separator = "\\";
	if (std::string::npos != path.rfind("/")) separator = "/";
  
	if (std::string::npos ==	path.rfind(separator)) {
  
		path = buffer;

 	} else {
  
		path.erase(path.rfind(separator)); // find start path
  
		char *temp = (char *)path.c_str();

		if ( !((temp[0] == separator[0]) || (temp[1] == ':')) ) {
  
			file = buffer;
			file += separator;
			file += path;
  
			path = file.c_str();
  
 		}
 	}
 
 	if (argc >=2 )	SendToGBA(argv[1]);

	Fl::add_idle((void (*)(void*))GBAConsole);

	return Fl::run();

}