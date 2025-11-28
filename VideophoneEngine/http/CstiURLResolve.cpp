// ////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiURLResolve
//
//  File Name:	CstiURLResolve.cpp
//
//  Owner:		Ting-Yu Yang
//
//	Abstract:
//		The CstiURLResolve implements the functions needed for resolving
//		URL address for http services
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stiTrace.h"
#include "stiTaskInfo.h"
#include "CstiHTTPTask.h"
#include "CstiURLResolve.h"

//
// Constants
//

//
// Typedefs
//

//
// Globals
//

//
// Forward Declarations
//

////////////////////////////////////////////////////////////////////////////////
//; CstiURLResolve::CstiURLResolve
//
// Description: Default constructor
//
// Abstract:
//
// Returns:
//	none
//
CstiURLResolve::CstiURLResolve (
	PstiObjectCallback pfnAppCallback,
	size_t CallbackParam,
	CstiHTTPTask *pHTTPTask)
	:
	CstiEventQueue (stiURL_TASK_NAME),
	m_pHTTPTask (pHTTPTask)
{
	stiLOG_ENTRY_NAME (CstiURLResolve::CstiURLResolve);
} // end CstiURLResolve::CstiURLResolve


////////////////////////////////////////////////////////////////////////////////
//; CstiURLResolve::httpUrlResolve
//
// Description: Tells the task to start resolving the specific URL address
//
// Abstract:
//
void CstiURLResolve::httpUrlResolve (
	SHTTPPostServiceData *pstPostServiceData)
{
	stiLOG_ENTRY_NAME (CstiURLResolve::HTTPUrlResolve);

	PostEvent ([this, pstPostServiceData]
		{
			if (nullptr != pstPostServiceData)
			{
				EURLParseResult eURLResult = eINVALID_URL;
				auto url = pstPostServiceData->postInfo.url;
				auto urlAlt = pstPostServiceData->postInfo.urlAlt;

				stiDEBUG_TOOL (g_stiHTTPTaskDebug,
					stiTrace ("URLResolvingHandle:: Resolving URL %s...\n", url.c_str ());
				); // stiDEBUG_TOOL

				// Was there a URL specified?
				if (!url.empty ())
				{
					// Yes! Ask the service provider to parse the URL.

					eURLResult = (pstPostServiceData->poHTTPService)->URLParse (
						url.c_str (),
						urlAlt.c_str (),
						&pstPostServiceData->ResolvedServerIPList,
						&pstPostServiceData->nPort,
						&pstPostServiceData->File);

					if (eDNS_CANCELLED == eURLResult)
					{
						stiDEBUG_TOOL (g_stiHTTPTaskDebug,
							stiTrace ("CstiURLResolve:: Service %p Cancelled\n", pstPostServiceData->poHTTPService);
						); // stiDEBUG_TOOL

						// free the memory
						m_pHTTPTask->HTTPServiceRemove (pstPostServiceData->poHTTPService);

						delete pstPostServiceData;
					}
					// Did it succeed?
					else if (eURL_OK == eURLResult && !pstPostServiceData->ResolvedServerIPList.empty ())
					{
						stiDEBUG_TOOL (g_stiHTTPTaskDebug,
							stiTrace ("CstiURLResolve:: task %d pid %d service %p post HTTPUrlResolveSuccess\n", stiOSTaskIDSelf (), getpid (), pstPostServiceData->poHTTPService);
						); // stiDEBUG_TOOL

						m_pHTTPTask->HTTPUrlResolveSuccess (pstPostServiceData);
					} // end if
					else
					{
						stiDEBUG_TOOL (g_stiHTTPTaskDebug,
							stiTrace ("CstiURLResolve:: task %d pid %d service %p post HTTPUrlResolveFail\n", stiOSTaskIDSelf (), getpid (), pstPostServiceData->poHTTPService);
						); // stiDEBUG_TOOL

						m_pHTTPTask->HTTPUrlResolveFail (pstPostServiceData);

					} // end else
				} // end if
			}

			stiDEBUG_TOOL (g_stiHTTPTaskDebug,
				stiTrace ("\n");
			);
		});
}
