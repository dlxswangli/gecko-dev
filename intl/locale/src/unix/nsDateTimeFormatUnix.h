
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef nsDateTimeFormatUnix_h__
#define nsDateTimeFormatUnix_h__


#include "nsIDateTimeFormat.h"


class nsDateTimeFormatUnix : public nsIDateTimeFormat {

public: 
  NS_DECL_ISUPPORTS 

  // performs a locale sensitive date formatting operation on the time_t parameter
  // locale RFC1766 (e.g. "en-US"), caller should allocate the buffer, outLen is in/out
  NS_IMETHOD FormatTime(const nsString& locale, 
                        const nsDateFormatSelector  dateFormatSelector, 
                        const nsTimeFormatSelector timeFormatSelector, 
                        const time_t  timetTime, 
                        PRUnichar *stringOut, 
                        PRUint32 *outLen); 

  // performs a locale sensitive date formatting operation on the struct tm parameter
  // locale RFC1766 (e.g. "en-US"), caller should allocate the buffer, outLen is in/out
  NS_IMETHOD FormatTMTime(const nsString& locale, 
                        const nsDateFormatSelector  dateFormatSelector, 
                        const nsTimeFormatSelector timeFormatSelector, 
                        const struct tm*  tmTime, 
                        PRUnichar *stringOut, 
                        PRUint32 *outLen); 

  nsDateTimeFormatUnix() {NS_INIT_REFCNT();};
};

#endif  /* nsDateTimeFormatUnix_h__ */
