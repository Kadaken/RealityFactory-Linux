/************************************************************************************//**
 * @file CMp3.cpp
 * @brief CMp3Manager class implementation
 ****************************************************************************************/

#include "RabidFramework.h"				// The One True Header

#ifndef _WIN32

CMp3Manager::CMp3Manager()
{
	Active = false;
	m_Video = NULL;
	Length = 0;
	Loop = false;
}
CMp3Manager::~CMp3Manager() {}
void CMp3Manager::OpenMediaFile(LPSTR) { Active = false; }
void CMp3Manager::PlayMp3(long, geBoolean loop) { Loop = loop != GE_FALSE; }
void CMp3Manager::StopMp3() { Active = false; }
void CMp3Manager::Refresh() {}
void CMp3Manager::SetVolume(long) {}

#else

// changed RF064
/* ------------------------------------------------------------------------------------ */
//	Constructor
/* ------------------------------------------------------------------------------------ */
CMp3Manager::CMp3Manager()
{
	m_Video = NULL;
    Active = false;
	Loop = false;
}

/* ------------------------------------------------------------------------------------ */
//	Destructor
/* ------------------------------------------------------------------------------------ */
CMp3Manager::~CMp3Manager()
{
	if(m_Video != NULL)
		MCIWndDestroy(m_Video);
}

/* ------------------------------------------------------------------------------------ */
//	OpenMediaFile
//
//	This function opens and renders the specified media file.
//	File..Open has been selected
/* ------------------------------------------------------------------------------------ */
void CMp3Manager::OpenMediaFile(LPSTR szFile)
{
	m_Video = MCIWndCreate(CCD->Engine()->WindowHandle(),
		// AfxGetInstanceHandle(),
		NULL,
		WS_CHILD | MCIWNDF_NOMENU, szFile);

	if(m_Video)
	{
		MCIWndUseTime(m_Video);
		Length = MCIWndGetLength(m_Video);
	}
} // OpenMediaFile

/* ------------------------------------------------------------------------------------ */
//	Refresh
/* ------------------------------------------------------------------------------------ */
void CMp3Manager::Refresh()
{
	if(Active)
	{
		if(Loop)
		{
			LONG Pos = MCIWndGetPosition(m_Video);

			if(Pos > 0)
			{
				if((Pos+5) >= Length)
				{
					MCIWndHome(m_Video);
					MCIWndPlay(m_Video);
				}
			}
		}
	}
}

/* ------------------------------------------------------------------------------------ */
//	PlayMp3
/* ------------------------------------------------------------------------------------ */
void CMp3Manager::PlayMp3(long volume, geBoolean loop)
{
	if(m_Video)
	{
		MCIWndPlay(m_Video);
		Loop = loop;
		Active = true;
		SetVolume(volume);
	}
} // PlayMp3

/* ------------------------------------------------------------------------------------ */
//	StopMp3
/* ------------------------------------------------------------------------------------ */
void CMp3Manager::StopMp3()
{
	if(Active)
		MCIWndStop(m_Video);

	return ;
} //StopMp3
// end change RF064

/* ------------------------------------------------------------------------------------ */
//	SetVolume
/* ------------------------------------------------------------------------------------ */
void CMp3Manager::SetVolume(long volume)
{
	if(m_Video)
	{
		MCIWndSetVolume(m_Video, volume);
	}
}


/* ----------------------------------- END OF FILE ------------------------------------ */

#endif
