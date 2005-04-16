/*
 * Copyright 2005 Paul Walker
 *
 * GNU General Public License
 * 
 * This file is part of: Kernel Emulation on Windows (keow)
 *
 * Keow is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Keow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Keow; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * an IO handler for console windows
 *
 * escape codes to match TERM=ansi80x25
 * which is a FreeBSD console and is simpler than TERM=linux
 * but still has most features we want.
 *
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


//based on FOREGROUND_* BACKGROUND_* COMMON_LVB_*
#define FOREGROUND_MASK 0x000F
#define BACKGROUND_MASK 0x00F0
#define COMMON_LVB_MASK 0xFF00

//////////////////////////////////////////////////////////////////////////////////


ConsoleIOHandler::ConsoleIOHandler(int nDeviceNum, bool initialise)
: IOHandler(sizeof(ConsoleIOHandler))
, m_nTerminalNumber(nDeviceNum)
, m_DeviceData( pKernelSharedData->TerminalDeviceData[nDeviceNum] )
{
	ktrace("console using CONIN$,CONOUT$\n");

	//open console

	//one value must be set for all handles (why?)
	DWORD dwConsoleMode = ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT;

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


bool ConsoleIOHandler::Open(Path& filepath, DWORD access, DWORD ShareMode, DWORD disposition, DWORD flags)
{
	//was always open
	//TODO: move constructor stuff here. Do we need to?
	return true;
}
bool ConsoleIOHandler::Close()
{
	CloseHandle(m_HandleOut); //additional one for us to free
	return IOHandler::Close();
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

	do //while(canblock) 
	{
retry:
		if(!canblock)
		{
			if(!PeekConsoleInput(m_Handle, &buf, 1, &dwRead))
				continue;
			if(dwRead==0)
				continue;
		}

		if(!ReadConsoleInput(m_Handle, &buf, 1, &dwRead))
			continue;
		if(dwRead==0)
			continue;

		//debug
		if(buf.EventType==KEY_EVENT) {
			ktrace("KEY_EVENT bKeyDown %d, vkey %d, ascii %d, ctrlkeys %d\n", buf.Event.KeyEvent.bKeyDown, buf.Event.KeyEvent.wVirtualKeyCode, buf.Event.KeyEvent.uChar.AsciiChar, buf.Event.KeyEvent.dwControlKeyState);
		}

		//Ctrl-C is a special case
		if(buf.EventType==KEY_EVENT
		&& buf.Event.KeyEvent.uChar.AsciiChar==CTRL('C'))
		{
			//Ctrl-C does not come as a bKeydown so handle it specially
			ktrace("Ctrl-C\n");
			c=CTRL('C');
			return true;
		}

		if(buf.EventType==KEY_EVENT
		&& buf.Event.KeyEvent.bKeyDown!=0 )
		{
			//some keys require a different code than NT uses, translate them

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

			case VK_HOME: // ESC [ H
				c=ESC;
				PushForNextRead("[H");
				break;
			case VK_END: // ESC [ F
				c=ESC;
				PushForNextRead("[F");
				break;

			case VK_NEXT: // ESC [ G
				c=ESC;
				PushForNextRead("[G");
				break;
			case VK_PRIOR: // ESC [ I
				c=ESC;
				PushForNextRead("[I");
				break;

			case VK_DELETE: // \177
				c='\177';
				break;
			case VK_INSERT: // ESC [ L
				c=ESC;
				PushForNextRead("[L");
				break;

			case VK_F1: // ESC [ M
			case VK_F2: // ESC [ N
			case VK_F3: // ESC [ O
			case VK_F4: // ESC [ P
			case VK_F5: // ESC [ Q
			case VK_F6: // ESC [ R
			case VK_F7: // ESC [ S
			case VK_F8: // ESC [ T
			case VK_F9: // ESC [ U
			case VK_F10: // ESC [ V
			case VK_F11: // ESC [ W
			case VK_F12: // ESC [ X
			case VK_F13: // ESC [ Y
			case VK_F14: // ESC [ Z
			case VK_F15: // ESC [ a
			case VK_F16: // ESC [ b
			case VK_F17: // ESC [ c
			case VK_F18: // ESC [ d
			case VK_F19: // ESC [ e
			case VK_F20: // ESC [ f
			case VK_F21: // ESC [ g
			case VK_F22: // ESC [ h
			case VK_F23: // ESC [ i
			case VK_F24: // ESC [ j
				{
					char extra[] = "[M";
					int fn = c - VK_F1;
					extra[1] = "-MNOPQRSTUVWXYZabcdefghij"[fn];
					PushForNextRead(extra);
					c=ESC;
				}
				break;

			//TODO: more special key codes


			default: //this bit handles 'normal' keys that generate characters
				if(buf.Event.KeyEvent.uChar.AsciiChar==0) {
					//we read something not suitable for returning, retry the read, even when non-blocking
					goto retry;
				} else {
					c = buf.Event.KeyEvent.uChar.AsciiChar;
					ktrace("key %d %c\n", c,c);
				}
				break;
			}
			return true;
		}
	} while(canblock);

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
		{ //blocking
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

		if((m_DeviceData.TermIOs.c_iflag & INLCR) && *p==NL)
			PushForNextRead("\x0d"); //CR
		else
		if((m_DeviceData.TermIOs.c_iflag & ICRNL) && *p==CR)
			PushForNextRead("\x0a"); //NL

		if((m_DeviceData.TermIOs.c_iflag & IUCLC))
			*p = tolower(*p);


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
		if(m_DeviceData.TermIOs.c_lflag & ISIG)
		{
			if(*p==m_DeviceData.TermIOs.c_cc[VINTR])
				SendSignal(pProcessData->PID, SIGINT);
			//others?
			//VQUIT
			//VERASE
			//VKILL
			//VEOF
			//VTIME
			//VMIN
			//VSWTC
			//VSTART
			//VSTOP
			//VSUSP
			//VEOL
			//VREPRINT
			//VDISCARD
			//VWERASE
			//VLNEXT
			//VEOL2 
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
	CONSOLE_SCREEN_BUFFER_INFO info;
	COORD pos;
	int firstY, screenWidth, screenHeight;

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
		default:
			//unknown value - end esc sequence
			ktrace("Implement console code: ESC %c\n", c);
			m_DeviceData.OutputState = 0;
			break;
		}
		break;

	case ESC<<8 | '[':
		//lots of sequences want this, so do it first
		GetConsoleScreenBufferInfo(m_HandleOut, &info);
		screenWidth = info.srWindow.Right - info.srWindow.Left + 1;
		screenHeight = info.srWindow.Bottom - info.srWindow.Top + 1;
		firstY = info.dwSize.Y - screenHeight - 1; //first line of actual screen (bottom-most lines of buffer)
		//ktrace("ESC [  .... %c   %d\n", c,m_DeviceData.OutputStateData[0]);

		switch(c)
		{
		case '=':
			m_DeviceData.OutputState = '['<<8 | '=';
			break;

		case '0': //collect numeric parameters
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
				//add this digit to what we have
				m_DeviceData.OutputStateData[cnt+1] *= 10;
				m_DeviceData.OutputStateData[cnt+1] += c-'0';
			}
			break;
		case ';': //number seperator
			++m_DeviceData.OutputStateData[0]; //next parameter
			break;

		case 'J': // ESC [ J
			{
				//clear to end of screen

				DWORD dwWritten;
				WORD len;

				pos = info.dwCursorPosition;
				len = screenWidth - pos.X;
				len += (info.dwSize.Y - pos.Y) * info.dwSize.X;
				FillConsoleOutputCharacter(m_HandleOut, ' ', len, pos, &dwWritten);
			}
			m_DeviceData.OutputState = 0;
			break;
		case 'K': // ESC [ K
			{
				//clear to end of line

				DWORD dwWritten;
				WORD len;

				pos = info.dwCursorPosition;
				len = screenWidth - pos.X;
				FillConsoleOutputCharacter(m_HandleOut, ' ', len, pos, &dwWritten);
			}
			m_DeviceData.OutputState = 0;
			break;

		case '`': // ESC [ n `
			{
				//move cursor to col n in current row
				int col = m_DeviceData.OutputStateData[1];

				pos.X = col-1;
				pos.Y = info.dwCursorPosition.Y;
				SetConsoleCursorPosition(m_HandleOut, pos);
			}
			m_DeviceData.OutputState = 0;
			break;
		case 'H': // ESC [ n;n H
			{
				//move cursor to row,col
				if(m_DeviceData.OutputStateData[0] == 0) {
					pos.X = 0;
					pos.Y = firstY;
				} else {
					pos.X = m_DeviceData.OutputStateData[2] - 1;
					pos.Y = m_DeviceData.OutputStateData[1] - 1 + firstY;
ktrace("cursor pos %d,%d  first %d\n", pos.X, pos.Y, firstY);
				}
				SetConsoleCursorPosition(m_HandleOut, pos);
			}
			m_DeviceData.OutputState = 0;
			break;

		case 'B': // ESC [ n B
			{
				//move cursor down
				pos.X = info.dwCursorPosition.X;
				if(m_DeviceData.OutputStateData[0] == 0)
					pos.Y = info.dwCursorPosition.Y + 1;
				else
					pos.Y = info.dwCursorPosition.Y + m_DeviceData.OutputStateData[1];
				SetConsoleCursorPosition(m_HandleOut, pos);
			}
			m_DeviceData.OutputState = 0;
			break;
		case 'A': // ESC [ A
			{
				//move cursor up
				pos.X = info.dwCursorPosition.X;
				if(m_DeviceData.OutputStateData[0] == 0)
					pos.Y = info.dwCursorPosition.Y - 1;
				else
					pos.Y = info.dwCursorPosition.Y - m_DeviceData.OutputStateData[1];
				SetConsoleCursorPosition(m_HandleOut, pos);
			}
			m_DeviceData.OutputState = 0;
			break;
		case 'C': // ESC [ C
			{
				//move cursor right
				if(m_DeviceData.OutputStateData[0] == 0)
					pos.X = info.dwCursorPosition.X + 1;
				else
					pos.X = info.dwCursorPosition.X + m_DeviceData.OutputStateData[1];
				pos.Y = info.dwCursorPosition.Y;
				SetConsoleCursorPosition(m_HandleOut, pos);
			}
			m_DeviceData.OutputState = 0;
			break;
		case 'D': // ESC [ D
			{
				//move cursor left
				if(m_DeviceData.OutputStateData[0] == 0)
					pos.X = info.dwCursorPosition.X - 1;
				else
					pos.X = info.dwCursorPosition.X - m_DeviceData.OutputStateData[1];
				pos.Y = info.dwCursorPosition.Y;
				SetConsoleCursorPosition(m_HandleOut, pos);
			}
			m_DeviceData.OutputState = 0;
			break;

		case 'E': // ESC [ E
			{
				//newline
				WriteConsole(m_HandleOut, "\r\n", 2, &dwWritten, NULL);
			}
			m_DeviceData.OutputState = 0;
			break;

		case 'P': // ESC [ P
			{
				//delete character at current pos

				//move line contents left
				SMALL_RECT moveRect;
				CHAR_INFO fill;
				moveRect.Left=info.dwCursorPosition.X + 1;
				moveRect.Right=info.dwSize.X;
				moveRect.Top=info.dwCursorPosition.Y;
				moveRect.Bottom=info.dwCursorPosition.Y;
				pos.X = info.dwCursorPosition.X;
				pos.Y = moveRect.Top;
				fill.Attributes=info.wAttributes;
				fill.Char.AsciiChar=' '; //space
				ScrollConsoleScreenBuffer(m_HandleOut, &moveRect, NULL, pos, &fill);
			}
			m_DeviceData.OutputState = 0;
			break;
		case 'M': // ESC [ M
			{
				//delete line at current pos

				//move lines up
				SMALL_RECT moveRect;
				CHAR_INFO fill;
				moveRect.Left=0;
				moveRect.Right=info.dwSize.X;
				moveRect.Top=info.dwCursorPosition.Y + 1;
				moveRect.Bottom=info.dwSize.Y;
				pos.X = 0;
				pos.Y = info.dwCursorPosition.Y;
				fill.Attributes=info.wAttributes;
				fill.Char.AsciiChar=' '; //space
				ScrollConsoleScreenBuffer(m_HandleOut, &moveRect, NULL, pos, &fill);
			}
			m_DeviceData.OutputState = 0;
			break;

		case '@': // ESC [ @
			{
				//insert character at current pos

				//move line contents right
				SMALL_RECT moveRect;
				CHAR_INFO fill;
				moveRect.Left=info.dwCursorPosition.X;
				moveRect.Right=info.dwSize.X;
				moveRect.Top=info.dwCursorPosition.Y;
				moveRect.Bottom=info.dwCursorPosition.Y;
				pos.X = info.dwCursorPosition.X + 1;
				pos.Y = moveRect.Top;
				fill.Attributes=info.wAttributes;
				fill.Char.AsciiChar=' '; //space
				ScrollConsoleScreenBuffer(m_HandleOut, &moveRect, NULL, pos, &fill);
			}
			m_DeviceData.OutputState = 0;
			break;
		case 'L': // ESC [ L
			{
				//insert line at current pos

				//move lines down
				SMALL_RECT moveRect;
				CHAR_INFO fill;
				moveRect.Left=0;
				moveRect.Right=info.dwSize.X;
				moveRect.Top=info.dwCursorPosition.Y;
				moveRect.Bottom=info.dwSize.Y;
				pos.X = 0;
				pos.Y = info.dwCursorPosition.Y + 1;
				fill.Attributes=info.wAttributes;
				fill.Char.AsciiChar=' '; //space
				ScrollConsoleScreenBuffer(m_HandleOut, &moveRect, NULL, pos, &fill);
			}
			m_DeviceData.OutputState = 0;
			break;

		case 'm': // ESC [ n m
			{
				//set attributes
				int attr = m_DeviceData.OutputStateData[1];
				switch(attr)
				{
				case 0: //     reset all attributes to their defaults
					//white text, black background
					info.wAttributes &= ~FOREGROUND_MASK;
					info.wAttributes |= FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN;
					break;

				case 1: //     set bold
					info.wAttributes |= FOREGROUND_INTENSITY;
					break;

				case 2: //     set half-bright (simulated with color on a color display)
					info.wAttributes &= ~FOREGROUND_INTENSITY;
					break;

				case 5: //     set blink
					break;

				case 7: //     set reverse video
					info.wAttributes |= COMMON_LVB_REVERSE_VIDEO;
					break;

				}

				SetConsoleTextAttribute(m_HandleOut, info.wAttributes);
			}
			m_DeviceData.OutputState = 0;
			break;

		default:
			// ESC [ n;n;n; X (unknown value) - end esc sequence
			ktrace("Implement console code: ESC [ n %c\n", c);
			m_DeviceData.OutputState = 0;
			break;
		}
		break;

	case '['<<8 | '=':  // ESC [ = 
		switch(c)
		{
		case '0': //collect numeric parameters
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
				//add this digit to what we have
				m_DeviceData.OutputStateData[cnt+1] *= 10;
				m_DeviceData.OutputStateData[cnt+1] += c-'0';
			}
			break;
		case ';': //number seperator
			++m_DeviceData.OutputStateData[0];
			break;

		case 'C': // ESC [ = n C
			{
				int n = m_DeviceData.OutputStateData[1];

				CONSOLE_CURSOR_INFO cinfo;
				GetConsoleCursorInfo(m_HandleOut, &cinfo);

				switch(n)
				{
				case 0:	//cursor normal
					cinfo.bVisible = 1;
					break;
				case 1:	//cursor visible  -where is hide?
					cinfo.bVisible = 1;
					break;
				}

				SetConsoleCursorInfo(m_HandleOut, &cinfo);
			}
			m_DeviceData.OutputState = 0;
			break;

		default:
			// ESC [ = n;n;n; X (unknown value) - end esc sequence
			ktrace("Implement console code: ESC [ = n %c\n", c);
			m_DeviceData.OutputState = 0;
			break;
		}
		break;
	}

	return true;
}

bool ConsoleIOHandler::Write(const void* address, DWORD size, DWORD *pWritten)
{
	if(size!=0)
	{
		const char *p = (const char*)address;
		const char *pEnd = p+size;
		for(; p<pEnd; ++p)
			WriteChar(*p);
	}
	if(pWritten)
		*pWritten=size;
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
	case TCGETS: //get stty stuff
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
	case TCSETS: //set stty stuff
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

	case TIOCGPGRP: //get process group
		{
			if(!data)
				return -EFAULT;
			*((linux::pid_t*)data) = m_DeviceData.ProcessGroup;
			return 0;
		}
		break;
	case TIOCSPGRP: //set process group
		{
			if(!data)
				return -EFAULT;
			m_DeviceData.ProcessGroup = *((linux::pid_t*)data);
			return 0;
		}
		break;

	case TIOCGWINSZ: //window size
		{
			linux::winsize *pWS = (linux::winsize *)data;
			CONSOLE_SCREEN_BUFFER_INFO Info;
			if(!GetConsoleScreenBufferInfo(m_HandleOut, &Info))
			{
				ktrace("Cannot get screen buffer info, err %ld\n", GetLastError());
				return Win32ErrToUnixError(GetLastError());
			}
			pWS->ws_col = Info.dwSize.X; //always full width
			pWS->ws_row = Info.srWindow.Bottom - Info.srWindow.Top + 1; //window height
			pWS->ws_xpixel = 0;//Info.srWindow.Right - Info.srWindow.Left;
			pWS->ws_ypixel = 0;//Info.srWindow.Bottom - Info.srWindow.Top;
			ktrace("console get size %d,%d\n", pWS->ws_col, pWS->ws_row);
			return 0;
		}
		break;
	case TIOCSWINSZ: //window size
		{
			linux::winsize *pWS = (linux::winsize *)data;
			ktrace("console set size %d,%d\n", pWS->ws_col, pWS->ws_row);
			CONSOLE_SCREEN_BUFFER_INFO Info;
			if(!GetConsoleScreenBufferInfo(m_HandleOut, &Info))
			{
				ktrace("Cannot get screen buffer info, err %ld\n", GetLastError());
				return Win32ErrToUnixError(GetLastError());
			}
			COORD sz;
			sz.X = pWS->ws_col;
			sz.Y = Info.dwSize.Y - 1; //keep our history
			if(!SetConsoleScreenBufferSize(m_HandleOut, sz))
			{
				ktrace("Cannot set screen buffer size, err %ld\n", GetLastError());
				return Win32ErrToUnixError(GetLastError());
			}
			SMALL_RECT sr;
			sr.Left = 0;
			sr.Right = sz.X - 1;
			sr.Bottom = sz.Y - 1;
			sr.Top = sr.Bottom - (pWS->ws_row - 1);
			if(!SetConsoleWindowInfo(m_HandleOut, true, &sr))
			{
				ktrace("Cannot set screen window size, err %ld\n", GetLastError());
				return Win32ErrToUnixError(GetLastError());
			}
			return 0;
		}
		break;

	case TCXONC: //TODO:
		return 0;

	default:
		ktrace("IMPLEMENT sys_ioctl 0x%lx for ConsoleIOHandler\n", request);
		return -ENOTTY;
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

bool ConsoleIOHandler::CanRead()
{
	//any pre-recorded input to send?
	if(m_DeviceData.InputState > 0) 
		return true;
	DWORD dwRead;
	INPUT_RECORD buf;
	if(!PeekConsoleInput(m_Handle, &buf, 1, &dwRead))
		return false;
ktrace("console::CanRead dwRead = %lx\n", dwRead);
	return dwRead != 0;
}

bool ConsoleIOHandler::CanWrite()
{
	//always except to be able to write to the console?
	return true;
}

bool ConsoleIOHandler::HasException()
{
	//TODO: what could this be?
	return false;
}



int ConsoleIOHandler::GetDirEnts64(linux::dirent64 *, int maxbytes)
{
	return 0;
}

ULONGLONG ConsoleIOHandler::Length()
{
	return 0L;
}

ULONGLONG ConsoleIOHandler::Seek(ULONGLONG offset, DWORD method)
{
	return 0L;
}
