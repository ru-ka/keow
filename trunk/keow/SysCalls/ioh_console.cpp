/*
 * and IO handler for console windows
 */
#include "kernel.h"
#include "iohandler.h"
#include "winuser.h"


#define CTRL(ch)  (ch-'A'+1)

#define CR 0x0d
#define NL 0x0a //NewLine/LineFeed
#define LF 0x0a //NewLine/LineFeed
#define FF 0x0c //formfeed
#define VT 0x0b //vertical tab
#define TAB 0x09
#define ESC 0x1b
#define BACKSPACE 0x08
#define SO 0x0e //active G1 character set
#define SI 0x0f //active G0 character set
#define CAN 0x18
#define SUB 0x1a 
#define DEL 0x7f 



ConsoleIOHandler::ConsoleIOHandler(int nDeviceNum, bool initialise)
: IOHandler(sizeof(ConsoleIOHandler))
, m_nTerminalNumber(nDeviceNum)
, m_DeviceData( pKernelSharedData->TerminalDeviceData[nDeviceNum] )
{
	ktrace("console using CONIN$,CONOUT$\n");

	//open console

	//one value must be set for all handles (why?)
	DWORD dwConsoleMode = ENABLE_PROCESSED_OUTPUT;

	HANDLE h = CreateFile("CONIN$", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	//can inherit handle....
	if(h==INVALID_HANDLE_VALUE)
	{
		if(m_Handle!=INVALID_HANDLE_VALUE)
			CloseHandle(m_Handle);
		m_Handle = INVALID_HANDLE_VALUE;
	}
	else 
	{
		DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &m_Handle, 0, TRUE, DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);
		//SetConsoleMode(m_Handle, dwConsoleMode);
	}

	h = CreateFile("CONOUT$", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	//can inherit handle....
	if(h==INVALID_HANDLE_VALUE)
	{
		if(m_HandleOut!=INVALID_HANDLE_VALUE)
			CloseHandle(m_HandleOut);
		m_HandleOut = INVALID_HANDLE_VALUE;
	}
	else
	{
		DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &m_HandleOut, 0, TRUE, DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);
		//SetConsoleMode(m_HandleOut, dwConsoleMode);
	}

	//init new device?
	if(initialise)
	{
		SetConsoleMode(m_Handle, dwConsoleMode);
		SetConsoleMode(m_HandleOut, dwConsoleMode);

		m_DeviceData.TermIOs.c_iflag = IGNBRK|IGNPAR|IXOFF;
		m_DeviceData.TermIOs.c_oflag = 0;
		m_DeviceData.TermIOs.c_cflag = CS8;
		m_DeviceData.TermIOs.c_lflag = ECHO;
		m_DeviceData.TermIOs.c_line = N_TTY;
		memset(m_DeviceData.TermIOs.c_cc, 0, sizeof(m_DeviceData.TermIOs.c_cc));
		StringCbCopy((char*)m_DeviceData.TermIOs.c_cc, sizeof(m_DeviceData.TermIOs.c_cc), INIT_C_CC); //default special chars from kernel files

		//make normal chars work - reverse CR/LF
		//m_DeviceData.TermIOs.c_iflag |= INLCR | ICRNL;

		m_DeviceData.ProcessGroup = pProcessData->ProcessGroupPID;

		m_DeviceData.InputState=0;
		m_DeviceData.OutputState=0;

	}
}


bool ConsoleIOHandler::PushForNextRead(const char * pString) 
{
	int len = strlen(pString);
	if(m_DeviceData.InputState+len >= sizeof(m_DeviceData.InputState))
		return false; //no room in buffer

	if(len==0)
		return true; //nothing to do

	//push in reverse order because we pull from the end
	const char *p = &pString[len-1];
	while(p>=pString)
	{
		m_DeviceData.InputStateData[m_DeviceData.InputState] = *p;
		++m_DeviceData.InputState;
		--p;
	}
	return true;
}


bool ConsoleIOHandler::ReadChar(bool canblock, char &c)
{
//	bool ok;
	DWORD dwRead;

	//any pre-recorded input to send?
	if(m_DeviceData.InputState > 0) 
	{
		--m_DeviceData.InputState;
		c = m_DeviceData.InputStateData[m_DeviceData.InputState];
		return true;
	}


	INPUT_RECORD buf;

//	ok = true;

	while(canblock) 
	{
		if(!PeekConsoleInput(m_Handle, &buf, 1, &dwRead))
			continue;
		if(dwRead==0)
			continue;

		if(!ReadConsoleInput(m_Handle, &buf, 1, &dwRead))
			continue;
		if(dwRead==0)
			continue;

		if(buf.EventType==KEY_EVENT
		&& buf.Event.KeyEvent.bKeyDown!=0 )
		{
			switch(buf.Event.KeyEvent.wVirtualKeyCode)
			{
			case VK_RETURN:
				c=NL;
				break;

			case VK_UP: // ESC [ A
				c=ESC;
				PushForNextRead("[A");
				break;
			case VK_DOWN: // ESC [ B
				c=ESC;
				PushForNextRead("[B");
				break;
			case VK_LEFT: // ESC [ D
				c=ESC;
				PushForNextRead("[D");
				break;
			case VK_RIGHT: // ESC [ C
				c=ESC;
				PushForNextRead("[C");
				break;

			case VK_F1: // ESC [[ A
				c=ESC;
				PushForNextRead("[[A");
				break;
			case VK_F2: // ESC [[ E
				c=ESC;
				PushForNextRead("[[E");
				break;

			//TODO: more key codes

			default:
				c = buf.Event.KeyEvent.uChar.AsciiChar;
				break;
			}
			return true;
		}
	}
	return false;
}


bool ConsoleIOHandler::Read(void* address, DWORD size, DWORD *pRead)
{
	bool ok;
	DWORD cnt = 0;
	char* p = (char*)address;

	if(size==0)
	{
		if(pRead)
			*pRead=0;
		return true;
	}

	ok=true;
	do {
		if((m_Flags&O_NONBLOCK) || cnt>0)
		{
			if(!ReadChar(false, *p))
				break;
		}
		else
		{
			if(!ReadChar(true, *p))
			{
				ok = false;
				break;
			}
		}


		//Input settings: c_iflag
		//#define IGNBRK	0000001
		//#define BRKINT	0000002
		//#define IGNPAR	0000004
		//#define PARMRK	0000010
		//#define INPCK	0000020
		//#define ISTRIP	0000040
		//#define INLCR	0000100
		//#define IGNCR	0000200
		//#define ICRNL	0000400
		//#define IUCLC	0001000
		//#define IXON	0002000
		//#define IXANY	0004000
		//#define IXOFF	0010000
		//#define IMAXBEL	0020000
		if((m_DeviceData.TermIOs.c_iflag & ISTRIP))
			*p &= 0x3F; //make 7-bit
		if((m_DeviceData.TermIOs.c_iflag & IGNCR) && *p==CR)
			continue;
		//if((m_DeviceData.TermIOs.c_iflag & INLCR) && *p==NL)
		//	*p = CR;
		//else
		//if((m_DeviceData.TermIOs.c_iflag & ICRNL) && *p==CR)
		//	*p = NL;

		//Local settings: c_lflag;
		//#define ISIG	0000001
		//#define ICANON	0000002
		//#define XCASE	0000004
		//#define ECHO	0000010
		//#define ECHOE	0000020
		//#define ECHOK	0000040
		//#define ECHONL	0000100
		//#define NOFLSH	0000200
		//#define TOSTOP	0000400
		//#define ECHOCTL	0001000
		//#define ECHOPRT	0002000
		//#define ECHOKE	0004000
		//#define FLUSHO	0010000
		//#define PENDIN	0040000
		//#define IEXTEN	0100000
		if((m_DeviceData.TermIOs.c_lflag & ISIG)==0)
		{
			if(*p==m_DeviceData.TermIOs.c_cc[VINTR])
				SendSignal(pProcessData->PID, SIGINT);
			//others?
		}
		if(m_DeviceData.TermIOs.c_lflag & ECHO)
		{
			if(m_DeviceData.TermIOs.c_lflag & ECHOE)
				this->Write("\b \b", 3, NULL); //backspace-space-backspace
			else
				this->Write(p, 1, NULL);
		}

		//Output settings: c_oflag;
		//not for input :-)

		cnt++;
		p++;
	} while(cnt<size);

	if(pRead)
		*pRead = cnt;

	return ok;
}

bool ConsoleIOHandler::WriteChar(char c)
{
	DWORD dwWritten;
	const int ignore_next_char = 0xFFF;

	//some characters are always special
	switch(c)
	{
	case CTRL('G'):
	case BACKSPACE:
	case TAB:
	case CR:
		WriteConsole(m_HandleOut, &c, 1, &dwWritten, NULL);
		return true;

	case LF:
	case FF:
	case VT:
		c=LF;
		WriteConsole(m_HandleOut, &c, 1, &dwWritten, NULL);
		return true;

	case SO:
	case SI:
	case DEL:
		//ignore
		return true;
	}

	//state machine
	switch(m_DeviceData.OutputState)
	{
	case 0: //no special state
		if(c==ESC)
		{
			m_DeviceData.OutputState = ESC;
		}
		else
		if(c==0x9B) //CSI
		{
			m_DeviceData.OutputState = ESC<<8 | '[';
			memset(m_DeviceData.OutputStateData, 0, sizeof(m_DeviceData.OutputStateData));
		}
		else
		{
			switch(c)
			{
			case LF:
			case FF:
			case VT:
				c=LF;
				WriteConsole(m_HandleOut, &c, 1, &dwWritten, NULL);
				break;
			default:
				WriteConsole(m_HandleOut, &c, 1, &dwWritten, NULL);
				break;
			}
		}
		break;

	case ESC:
		switch(c)
		{
		case '[':
			m_DeviceData.OutputState = ESC<<8 | '[';
			memset(m_DeviceData.OutputStateData, 0, sizeof(m_DeviceData.OutputStateData));
			break;
		case 'c':
			//ESC c  = reset
			m_DeviceData.OutputState = 0;
			break;
		case 'D':
			//ESC D  = linefeed
			c=LF;
			WriteConsole(m_HandleOut, &c, 1, &dwWritten, NULL);
			m_DeviceData.OutputState = 0;
			break;
		case 'E':
			//ESC E  = newline
			c=NL;
			WriteConsole(m_HandleOut, &c, 1, &dwWritten, NULL);
			m_DeviceData.OutputState = 0;
			break;
		case ESC:
			//resets state
			m_DeviceData.OutputState = ESC;
			break;
		case CAN:
		case SUB:
			//abort esc sequence
			m_DeviceData.OutputState = 0;
			break;
		}
		break;

	case ESC<<8 | '[':
		switch(c)
		{
		case '?':
			//ignore: ESC [ xxx  and  ESC [ ? xxx  are quivalent
			break;
		case '[':
			//ignore: ESC [ [ x
			m_DeviceData.OutputState = ignore_next_char;
			break;
		case ESC:
			//resets state
			m_DeviceData.OutputState = ESC;
			break;
		case CAN:
		case SUB:
			//abort esc sequence
			m_DeviceData.OutputState = 0;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				int cnt = m_DeviceData.OutputStateData[0];
				m_DeviceData.OutputStateData[cnt+1] += c-'0';
			}
			break;
		case ';':
			++m_DeviceData.OutputStateData[0];
			break;
		case '@':
			{
				//insert n blanks at current pos
				int cnt = m_DeviceData.OutputStateData[1];
				//move line contents right
				SMALL_RECT moveRect;
				COORD newPos;
				CONSOLE_SCREEN_BUFFER_INFO info;
				CHAR_INFO fill;
				GetConsoleScreenBufferInfo(m_HandleOut, &info);
				moveRect.Left=info.dwCursorPosition.X;
				moveRect.Right=info.dwSize.X;
				moveRect.Top=info.dwCursorPosition.Y;
				moveRect.Bottom=info.dwCursorPosition.Y;
				newPos.X = moveRect.Left + cnt;
				newPos.Y = moveRect.Top;
				fill.Attributes=0;
				fill.Char.AsciiChar=' '; //space
				ScrollConsoleScreenBuffer(m_HandleOut, &moveRect, NULL, newPos, &fill);

				m_DeviceData.OutputState = 0;
			}
			break;
		}
		break;

	case ignore_next_char:
		//ignore this char, and reset state
		m_DeviceData.OutputState = 0;
		break;		
	}

	return true;
}

bool ConsoleIOHandler::Write(const void* address, DWORD size, DWORD *pWritten)
{
	const char *p = (const char*)address;
	const char *pEnd = p+size;
	for(; p<pEnd; ++p)
		WriteChar(*p);
	if(pWritten)
		*pWritten+=size;
	return true;
}


IOHandler* ConsoleIOHandler::Duplicate(HANDLE hFromProcess, HANDLE hToProcess)
{
	ConsoleIOHandler *ionew = new ConsoleIOHandler(m_nTerminalNumber, false);
	if(ionew)
	{
		if(ionew->DupBaseData(*this,hFromProcess,hToProcess))
			return ionew;

		delete ionew;
	}
	return NULL;
}


DWORD ConsoleIOHandler::ioctl(DWORD request, DWORD data)
{
	switch(request)
	{
	case TCGETS:
		{
			linux::termios * arg = (linux::termios*)data;
			arg->c_iflag = m_DeviceData.TermIOs.c_iflag;
			arg->c_oflag = m_DeviceData.TermIOs.c_oflag;
			arg->c_cflag = m_DeviceData.TermIOs.c_cflag;
			arg->c_lflag = m_DeviceData.TermIOs.c_lflag;
			arg->c_line = m_DeviceData.TermIOs.c_line;
			memcpy(arg->c_cc, m_DeviceData.TermIOs.c_cc, NCCS);
			return 0;
		}
		break;

	case TCSETSW:
		//flush output
		//console io never buffered, so not required
		//...fall through...
	case TCSETSF:
		//flush input & output
		FlushConsoleInputBuffer(m_Handle);
		//...fall through...
	case TCSETS:
		{
			linux::termios * arg = (linux::termios*)data;
			m_DeviceData.TermIOs.c_iflag = arg->c_iflag;
			m_DeviceData.TermIOs.c_oflag = arg->c_oflag;
			m_DeviceData.TermIOs.c_cflag = arg->c_cflag;
			m_DeviceData.TermIOs.c_lflag = arg->c_lflag;
			m_DeviceData.TermIOs.c_line = arg->c_line;
			memcpy(m_DeviceData.TermIOs.c_cc, arg->c_cc, NCCS);
			return 0;
		}
		break;

	case TIOCGPGRP:
		{
			if(!data)
				return -EFAULT;
			*((linux::pid_t*)data) = m_DeviceData.ProcessGroup;
			return 0;
		}
		break;
	case TIOCSPGRP:
		{
			if(!data)
				return -EFAULT;
			m_DeviceData.ProcessGroup = *((linux::pid_t*)data);
			return 0;
		}
		break;

	case TIOCGWINSZ:
		{
			linux::winsize *pWS = (linux::winsize *)data;
			CONSOLE_SCREEN_BUFFER_INFO Info;
			//if(!GetConsoleScreenBufferInfo(m_Handle, &Info))
			if(!GetConsoleScreenBufferInfo(m_HandleOut, &Info))
			{
				ktrace("Cannot get screen buffer info, err %ld\n", GetLastError());
				return Win32ErrToUnixError(GetLastError());
			}
			pWS->ws_col = Info.srWindow.Right - Info.srWindow.Left + 1;
			pWS->ws_row = Info.srWindow.Bottom - Info.srWindow.Top + 1;
			pWS->ws_xpixel = Info.dwCursorPosition.X - Info.srWindow.Left + 1;
			pWS->ws_ypixel = Info.dwCursorPosition.Y - Info.srWindow.Top + 1;
			return 0;
		}
		break;
	case TIOCSWINSZ:
		{
			linux::winsize *pWS = (linux::winsize *)data;
			SMALL_RECT sr;
			sr.Left = 0;
			sr.Top = 0;
			sr.Right = pWS->ws_col - 1;
			sr.Bottom = pWS->ws_row - 1;
			if(!SetConsoleWindowInfo(m_HandleOut, true, &sr))
			{
				ktrace("Cannot set screen window size, err %ld\n", GetLastError());
				return Win32ErrToUnixError(GetLastError());
			}
			return 0;
		}
		break;

	default:
		ktrace("IMPLEMENT sys_ioctl 0x%lx for ConsoleIOHandler\n", request);
		return -ENOSYS;
	}
	return -EINVAL;
}


bool ConsoleIOHandler::Stat64(linux::stat64 * s)
{
	if(!s)
		return false;

	IOHandler::BasicStat64(s, S_IFCHR);

	return true;
}
