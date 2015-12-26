#include <npfunctions.h> /* which icludes the rest of the headers*/
#include <stddef.h>      /* needed for offsetof() */

NPNetscapeFuncs* sBrowserFuncs = NULL;

void
NPN_Status(NPP instance, const char* message)
{
	sBrowserFuncs->status(instance, message);
}

NPError
NPN_GetURL(NPP instance, const char* url, const char* target)
{
	return sBrowserFuncs->geturl(instance, url, target);
}

NPError OSCALL
NP_Initialize(NPNetscapeFuncs* bFuncs)
{
	sBrowserFuncs = bFuncs;

	return NPERR_NO_ERROR;
}

NPError OSCALL
NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
	if (pFuncs->size < (offsetof(NPPluginFuncs, setvalue) + sizeof(void*)))
		return NPERR_INVALID_FUNCTABLE_ERROR;

	pFuncs->newp = NPP_New;
	pFuncs->destroy = NPP_Destroy;
	pFuncs->setwindow = NPP_SetWindow;
	pFuncs->newstream = NPP_NewStream;
	pFuncs->destroystream = NPP_DestroyStream;
	pFuncs->asfile = NPP_StreamAsFile;
	pFuncs->writeready = NPP_WriteReady;
	pFuncs->write = NPP_Write;
	pFuncs->print = NPP_Print;
	pFuncs->event = NPP_HandleEvent;
	pFuncs->urlnotify = NPP_URLNotify;
	pFuncs->getvalue = NPP_GetValue;
	pFuncs->setvalue = NPP_SetValue;

	return NPERR_NO_ERROR;
}

NPError OSCALL
NP_Shutdown()
{
	return NPERR_NO_ERROR;
}
